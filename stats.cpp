#include "stats.h"
#include "crow.h"
#include <iterator>
#include <memory>
#include <regex>
#include <stdexcept>
#include <unistd.h>

std::string getDate() {
    std::string date = execCommand("date '+%b %d'", std::bitset<4>{0b0000});
    return date;
}

void getMetrics(Metrics& metric) {
    std::string date;
    if (Metrics::metricFlag) {
        date = getDate();
    }
    std::string info;
    std::string command;
    std::ifstream file("./messages.log");
    std::string line;
    std::regex pattern(R"(SRC=([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+))");

    if (!file.is_open()) {
        std::cerr << "Error opening file" << std::endl;
    }
    if (Metrics::metricFlag) {
        while (std::getline(file, line)) {
            if (line.find(date) != std::string::npos) {
                if (line.find("New HTTPS") != std::string::npos) {
                    std::smatch matches;
                    if (std::regex_search(line, matches, pattern) && matches.size() > 1) {
                        metric.setIP(matches[1].str());
                        metric.setDates(matches[1].str(), date);
                    }
                }
            }
        }
    } else {
        while (std::getline(file, line)) {
            if (line.find("New HTTPS") != std::string::npos) {
                std::smatch matches;
                if (std::regex_search(line, matches, pattern) && matches.size() > 1) {
                    metric.setIP(matches[1].str());
                    metric.setDates(matches[1].str(), line.substr(0, 6));
                }
            }
        }
    }
    file.close();
    Metrics::metricFlag = true;
    parseIP(metric);
}

void getStats(Stats& stats, crow::mustache::context& ctx) {
    std::string result;
#ifdef __APPLE__
    result = execCommand("sysctl -n machdep.cpu.brand_string hw.ncpu hw.physicalcpu hw.logicalcpu hw.memsize && sw_vers -productName && sw_vers -productVersion && "
                         "hostname && uptime && top -l 1 | awk '/PhysMem/{physMem=$0} /Network/{network=$0} /CPU/ && !cpu {cpu=$0} /Disk/ && !disk {disk=$0} END{print "
                         "physMem; print network; print cpu; print disk}'",
                         std::bitset<4>{0b0000});
    parseStats(stats, result, true, ctx);
    stats.disk = execCommand("df -h", std::bitset<4>{0b0010});
#else
    std::string command = "cat /proc/cpuinfo | grep 'model name' | uniq | awk -F': ' '{print $2}' ; "
                          "grep -c 'processor' /proc/cpuinfo ; "
                          "nproc --all ; "
                          "hostnamectl | grep 'Kernel' | awk -F: '{print $2}' ; "
                          "cat /proc/meminfo | grep 'MemFree' | awk -F': ' '{print $2/1024}' ; "
                          "cat /etc/os-release | grep '^NAME=' | awk -F'=' '{print $2}' ; "
                          "cat /etc/os-release | grep 'VERSION_ID' | awk -F'=' '{print $2}' ; "
                          "hostname ; "
                          "uptime ; "
                          "cat /proc/meminfo | grep 'MemTotal' | awk -F': ' '{print $2/1024}'";
    result = execCommand(command.c_str(), std::bitset<4>{0b0000});
    parseStats(stats, result, true, ctx);
    stats.memoryUsage = execCommand("free -m", std::bitset<4>{0b0110});
    ctx["memoryUsage"] = stats.memoryUsage;
    // stats.emplace_back("kernel", execCommand("hostnamectl | grep 'Kernel' | awk -F: '{print $2}'", std::bitset<4>{0b0000}));
    // stats.emplace_back("kernel", execCommand("mpstat -P ALL 1 | head -n 1", std::bitset<4>{0b0000}));
    stats.disk = execCommand("df -h", std::bitset<4>{0b0010});
    stats.network = execCommand("ip addr | grep -E '^[0-9]+:|inet '", std::bitset<4>{0b1000});
    ctx["network"] = stats.network;
#endif
}

std::string addCpuUsage(Stats& stats) {
    int parseResponse = 5;
    parseResponse += std::stoi(stats.cpucount);
    const std::string command{"mpstat -P ALL 1 | head -n " + std::to_string(parseResponse) + " | awk '/^$/ {found=1; next} found'"};
    stats.cpuUsage = execCommand(command.c_str(), std::bitset<4>{0b0010});
    return command;
}

std::string execCommand(const char* cmd, std::bitset<4> v) {
    std::array<char, 200> buffer;
    std::string result;
    std::string mountPoint;
    bool var = false;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    std::string tableHtml = "<table class='table-auto w-full'><tbody>";
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    try {
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            if (v.test(0)) {
                result += buffer.data();
                result += "<br>";
            } else if (v.test(3) && v.test(1)) {
                result += '\n';
                result += buffer.data();
                result += '\n';
            } else if (v.test(1)) {
                std::istringstream iss(buffer.data());
                std::vector<std::string> parts(std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>());
                tableHtml += "<tr>";
                for (const auto& part : parts) {
                    if (part == "Mounted" || part == "map") {
                        mountPoint = part;
                        var = true;
                    } else if (var && !v.test(2)) {
                        mountPoint += " " + part;
                        tableHtml += "<td>" + mountPoint + "</td>";
                        var = false;
                    } else if (v.test(2) && !var) {
                        tableHtml += "<td>#</td>";
                        tableHtml += "<td>" + part + "</td>";
                        var = true;
                    } else {
                        tableHtml += "<td>" + part + "</td>";
                    }
                }
                tableHtml += "</tr>";
            } else if (v.test(2)) {
                result = buffer.data();
                return result;
            } else if (v.test(3)) {
                result += "<div>";
                result += buffer.data();
                result += "</div>";
            } else {
                result += buffer.data();
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    if (v.test(1) && !v.test(3)) {
        tableHtml += "</tbody></table>";
        result = tableHtml;
    }
    return result;
}

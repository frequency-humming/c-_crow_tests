#include "stats.h"

std::string getContainerIds() {
    std::string containerIds = execCommand("docker ps -q", std::bitset<4>{0b0000});
    return containerIds;
}

void getNameandIP(std::vector<Stats>& docker) {
    std::vector<Stats> temp;
    for (const auto& stat : docker) {
        if (stat.getName() == "containerId") {
            std::string command =
                "docker inspect " + stat.getValue() + " -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}} {{.Name}}' | awk -F'/' '{print $1 $2}'";
            std::string dockerStats = execCommand(command.c_str(), std::bitset<4>{0b0000});
            temp.emplace_back("name", dockerStats);
        }
    }
    docker.insert(docker.end(), temp.begin(), temp.end());
}

void streamParser(const std::string& ids, std::vector<Stats>& docker) {
    std::istringstream iss(ids);
    std::string value;
    while (iss >> value) {
        docker.emplace_back("containerId", value);
    }
}
void streamParser(const std::string& results, std::vector<DockerConfig>& config, std::string containerid, bool boolean) {
    std::istringstream iss(results);
    std::string value;
    DockerConfig temp;
    temp.containerid = containerid;
    int count = 0;
    if (boolean) {
        while (iss >> value) {
            switch (count) {
                case 0:
                    temp.logpath = value;
                    break;
                case 1: {
                    std::string digits;
                    for (char ch : value) {
                        if (std::isdigit(ch)) {
                            digits += ch;
                        }
                    }
                    temp.port = digits;
                    break;
                }
                case 2: {
                    std::string digits;
                    for (char ch : value) {
                        if (std::isdigit(ch)) {
                            digits += ch;
                        }
                    }
                    temp.port += ":" + digits;
                    break;
                }
            }
            count++;
        }
        for (auto& c : config) {
            if (c.containerid == containerid) {
                c.logpath = temp.logpath;
                c.port = temp.port;
            }
        }
    } else {
        while (iss >> value) {
            switch (count) {
                case 0:
                    temp.ipaddress = value;
                    break;
                case 1: {
                    size_t slash = value.find('/');
                    if (slash != std::string::npos) {
                        temp.name = value.substr(slash + 1);
                    } else {
                        temp.name = value;
                    }
                    break;
                }
                case 2: {
                    size_t t = value.find('T');
                    if (t != std::string::npos) {
                        temp.created = value.substr(0, t);
                    } else {
                        temp.created = value;
                    }
                    break;
                }
                case 3:
                    temp.logtype = value;
                    break;
                case 4: {
                    if (value != "0") {
                        temp.memory = value;
                    } else {
                        temp.memory = "No Data";
                    }
                    break;
                }
            }
            count++;
        }
        config.emplace_back(temp);
    }

    // else if (part.find("map") == 0 || (part.length() >= 3 && part.substr(part.length() - 3) == "}]]")) {
    //                     for (char ch : part) {
    //                         if (std::isdigit(ch)) {
    //                             result += ch;
    //                         } else if (ch == '/') {
    //                             result += ':';
    //                         } else if (ch == ']' && index < 1) {
    //                             index++;
    //                         } else if (ch == ']' && index > 0) {
    //                             result += '\n';
    //                         }
    //                     }

    // case 7:
    //     std::cout << "mount_source: " << value << std::endl;
    //     temp.mount_source = value;
    //     break;
    // case 8:
    //     std::cout << "mount_destination: " << value << std::endl;
    //     temp.mount_destination = value;
    //     break;
}

void dockerHealth(std::vector<Stats>& docker) {
    std::string containerIds{getContainerIds()};
    if (!containerIds.empty()) {
        streamParser(containerIds, docker);
        getNameandIP(docker);
        Stats::setBoolean(true);
    }
}

void dockerStats(std::vector<Stats>& stats, std::vector<DockerConfig>& config) {
    std::string command;
    std::string dockerStats;
    for (const auto& stat : stats) {
        if (stat.getName() == "containerId") {
            command = "docker inspect " + stat.getValue() + " -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}} " +
                      " {{.Name}} {{.Created}} {{.HostConfig.LogConfig.Type}} " + "{{.HostConfig.Memory}} " + "{{range.Mounts}} " +
                      "{{.Type}} : {{ .Source }} destination: {{.Destination}}{{ end }}'";
            dockerStats = execCommand(command.c_str(), std::bitset<4>{0b1010});
            streamParser(dockerStats, config, stat.getValue(), false);
            command = "docker inspect " + stat.getValue() + " -f '{{.LogPath}} {{.HostConfig.PortBindings}}'";
            dockerStats = execCommand(command.c_str(), std::bitset<4>{0b1010});
            streamParser(dockerStats, config, stat.getValue(), true);
        }
    }
}

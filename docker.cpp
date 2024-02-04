#include "stats.h"

std::string Stats::agent;

std::string getContainerIds() {
    std::string containerIds = execCommand("docker ps -q", std::bitset<4>{0b0000});
    return containerIds;
}

void getNameandIP(Stats& docker) {
    std::string command;
    std::string dockerStats;
    if (!docker.containerID.empty()) {
        for (const auto& stat : docker.containerID) {
            command = "docker inspect " + stat + " -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}} {{.Name}}' | awk -F'/' '{print $1 $2}'";
            dockerStats = execCommand(command.c_str(), std::bitset<4>{0b0000});
            if (dockerStats.find("agent") != std::string::npos) {
                std::cout << "ecs-agent" << dockerStats << std::endl;
                Stats::setAgent(stat);
            }
            docker.containerInfo.emplace_back(dockerStats);
        }
    }
}

void streamParser(const std::string& ids, Stats& docker) {
    std::istringstream iss(ids);
    std::string value;
    while (iss >> value) {
        docker.containerID.emplace_back(value);
    }
}
void streamParser(const std::string& results, std::vector<DockerConfig>& config, std::string containerid, std::string& execute) {
    std::istringstream iss(results);
    std::string value;
    DockerConfig temp;
    temp.containerid = containerid;
    int count = 0;
    if (execute == "logpath") {
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
    } else if (execute == "ip") {
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
                        double num = std::stod(value);
                        num /= 1024 * 1024;
                        std::ostringstream oss;
                        oss << std::fixed << std::setprecision(2) << num;
                        temp.memory = oss.str() + " MB";
                    } else {
                        temp.memory = "No Data";
                    }
                    break;
                }
                case 5: {
                    if (!value.empty()) {
                        temp.mount_source = value;
                    }
                    break;
                }
                case 6: {
                    if (!value.empty()) {
                        temp.mount_destination = value;
                    }
                    break;
                }
            }
            count++;
        }
        config.emplace_back(temp);
    } else {
        while (iss >> value) {
            switch (count) {
                case 0: {
                    if (!value.empty()) {
                        temp.mount_source = value;
                    }
                    break;
                }
                case 1: {
                    if (!value.empty()) {
                        temp.mount_destination = value;
                    }
                    break;
                }
            }
            count++;
        }
        for (auto& c : config) {
            if (c.containerid == containerid) {
                c.mount_source = temp.mount_source;
                c.mount_destination = temp.mount_destination;
            }
        }
    }
}

void dockerHealth(Stats& docker) {
    std::string containerIds{getContainerIds()};
    if (!containerIds.empty()) {
        streamParser(containerIds, docker);
        getNameandIP(docker);
        Stats::setBoolean(true);
    }
}

void dockerStats(Stats& stats, std::vector<DockerConfig>& config) {
    std::string command;
    std::string dockerStats;
    std::string ex;
    for (const auto& stat : stats.containerID) {
        if (stat != Stats::getAgent()) {
            command = "docker inspect " + stat + " -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}} " +
                      " {{.Name}} {{.Created}} {{.HostConfig.LogConfig.Type}} {{.HostConfig.Memory}}'";
            dockerStats = execCommand(command.c_str(), std::bitset<4>{0b1010});
            ex = "ip";
            streamParser(dockerStats, config, stat, ex);
            command = "docker inspect " + stat + " -f '{{.LogPath}} {{.HostConfig.PortBindings}}'";
            dockerStats = execCommand(command.c_str(), std::bitset<4>{0b1010});
            ex = "logpath";
            streamParser(dockerStats, config, stat, ex);
            command = "docker inspect " + stat + " -f '{{range.Mounts}} {{.Source}} {{.Destination}}{{end}}'";
            dockerStats = execCommand(command.c_str(), std::bitset<4>{0b1010});
            ex = "mount";
            streamParser(dockerStats, config, stat, ex);
        }
    }
}

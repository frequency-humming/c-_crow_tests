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
    std::string containerId;
    while (iss >> containerId) {
        docker.emplace_back("containerId", containerId);
    }
}

void dockerHealth(std::vector<Stats>& docker) {
    std::string containerIds{getContainerIds()};
    if (!containerIds.empty()) {
        streamParser(containerIds, docker);
        getNameandIP(docker);
    }
}
// maybe change the bitset to an emun an use it as a string to make code and intention clear
// from here I can ping the ip for health check
// or docker stats

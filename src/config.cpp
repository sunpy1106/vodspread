#include "config.h"

#include "../include/global.h"

int parse_config_line(const string &line, string &key, string& value) {
    size_t pos = 0;
    string lineBuf = line;
    while ((pos = lineBuf.find(' ')) != string::npos) {
        lineBuf.erase(pos, 1);
    }
    while ((pos = lineBuf.find('\t')) != string::npos) {
        lineBuf.erase(pos, 1);
    }

    if (lineBuf[0] == '#')
        return -1;
    pos = lineBuf.find('#');
    if (pos != string::npos) {
        lineBuf = lineBuf.substr(0, pos);
    }
    pos = lineBuf.find('=');
    if (pos != string::npos) {
        key = lineBuf.substr(0, pos);
        value = lineBuf.substr(pos + 1);
    }
    return 0;

}

int read_vodspread_config(const string fileName, ConfigType &configInfo) {
    ifstream confFile(fileName.c_str());

    if (!confFile.is_open()) {
        cout << "\tFatal Error : Cannot open configure file "
                << fileName << endl;
        exit(-1);
    }

    string line;
    cout << "read config file " << fileName << endl;
    while (confFile.good()) {
        getline(confFile, line);
        string key, value;
        parse_config_line(line, key, value);
        if (key.size() == 0 || value.size() == 0)
            continue;
        cout << "set " << key << " = " << value << endl;
        if (key.compare("resourceNumber") == 0) {
            configInfo.resourceNumber = atoi(value.c_str());
        } else if (key.compare("subServerNum") == 0) {
            configInfo.subServerNum = atoi(value.c_str());
        } else if (key.compare("maxPlayLen") == 0) {
            configInfo.maxPlayLen = atoi(value.c_str());
        } else if (key.compare("minPlayLen") == 0) {
            configInfo.minPlayLen = atoi(value.c_str());
        } else if (key.compare("minLoad") == 0) {
            configInfo.minLoad = atoi(value.c_str());
        } else if (key.compare("period") == 0) {
            configInfo.period = atoi(value.c_str());
        } else if (key.compare("DRBeta") == 0) {
            configInfo.DRBeta = atoi(value.c_str()) / 1000.0;
        } else if (key.compare("lambda") == 0) {// set the lambda of LRFU
            configInfo.lambda = atoi(value.c_str()) / 1000.0;
        } else if (key.compare("loadThresh") == 0) {// set the lambda of LRFU
            configInfo.loadThresh = atoi(value.c_str()) / 1000.0;
        } else if (key.compare("minCapacity") == 0) {
            configInfo.minCapacity = atoi(value.c_str());
        } else if (key.compare("maxCapacity") == 0) {
            configInfo.maxCapacity = atoi(value.c_str());
        } else if (key.compare("minLoad") == 0) {
            configInfo.minLoad = atoi(value.c_str());
        } else if (key.compare("maxLoad") == 0) {
            configInfo.maxLoad = atoi(value.c_str());
        } else if (key.compare("serverPort") == 0) {
            configInfo.serverPort = value;
        } else if (key.compare("spreadAlgorithm") == 0) {
            configInfo.spreadAlgorithm = value;
        } else if (key.compare("logFile") == 0) {
            configInfo.logFile = value;
        }else if (key.compare("poissonLambda") == 0) {
            configInfo.poissonLambda = atoi(value.c_str()) ;
        } else if (key.compare("zipfParameter") == 0) {
            configInfo.zipfParameter = atoi(value.c_str()) ;
        } else if (key.compare("resourceNumber") == 0) {
            configInfo.resourceNumber = atoi(value.c_str());
        } else if (key.compare("clientNumber") == 0) {
            configInfo.clientNumber = atoi(value.c_str());
        } else if (key.compare("serverIpAddress") == 0) {
            configInfo.serverIpAddress = value;
        } else if (key.compare("requestListFile") == 0) {
            configInfo.requestListFile = value;
        } else {
            cout << "can't find  arg :" << key << endl;
        }

    }
    confFile.close();
    return 0;
}

int read_client_config(const string fileName, ConfigType &clientConfig) {
    ifstream confFile(fileName.c_str());

    if (!confFile.is_open()) {
        cout << "\tFatal Error : Cannot open configure file "
                << fileName << endl;
        exit(-1);
    }

    string line;
    cout << "read configure file " << fileName << endl;
    while (confFile.good()) {
        getline(confFile, line);
        string key, value;
        parse_config_line(line, key, value);
        if (key.size() == 0 || value.size() == 0)
            continue;
        cout << "set " << key << " = " << value << endl;
        if (key.compare("poissonLambda") == 0) {
            clientConfig.poissonLambda = atoi(value.c_str()) ;
        } else if (key.compare("zipfParameter") == 0) {
            clientConfig.zipfParameter = atoi(value.c_str()) ;
        } else if (key.compare("resourceNumber") == 0) {
            clientConfig.resourceNumber = atoi(value.c_str());
        } else if (key.compare("clientNumber") == 0) {
            clientConfig.clientNumber = atoi(value.c_str());
        } else if (key.compare("serverIpAddress") == 0) {
            clientConfig.serverIpAddress = value;
        } else if (key.compare("serverPort") == 0) {
            clientConfig.serverPort = value;
        } else if (key.compare("logFile") == 0) {
            clientConfig.logFile = value;
        } else if (key.compare("requestListFile") == 0) {
            clientConfig.requestListFile = value;
        } else {
            cout << "can't find this arg :" << key << endl;
        }

    }
    confFile.close();
    return 0;
}

int read_clientcircle_config(const string filename, double &zeta, double &sigma) {
    return 0;
}

static string& trim(string& s) {
    s.erase(0, s.find_first_not_of(" "));
    s.erase(s.find_last_not_of(" ") + 1);
    return s;
}

//read a  file

int read_config_file(const string filename, map<string, string>& conf_map) {
    ifstream infile;
    infile.open(filename.c_str());
    if (!infile) {
        err_msg("Can not open file %s", filename.c_str());
        return -1;
    }

    conf_map.clear();

    for (string line, key, value; getline(infile, line);) {
        int pos; //position of "="
        trim(line);
        if (line.empty()) continue;
        if (line[0] == '#') continue;
        pos = line.find_first_of('=');
        key = line.substr(0, pos);
        value = line.substr(pos + 1, line.length() - pos - 1);
        trim(key);
        trim(value);
        conf_map[key] = value;
    }
    return 0;
}


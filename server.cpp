// C libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
// C++ libraries
#include <iostream>
#include <sstream>
#include <string>
// Network system libraries
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

const int PORT = 8080;
const char *html_header = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\nConnection: close\r\n\r\n";
const char *css_header = "HTTP/1.1 200 OK\r\nContent-Type: text/css; charset=UTF-8\r\nConnection: close\r\n\r\n";
const char *java_header = "HTTP/1.1 200 OK\r\nContent-Type: application/javascript; charset=UTF-8\r\nConnection: close\r\n\r\n";
const char *png_header = "HTTP/1.1 200 OK\r\nContent-Type: image/png\r\nConnection: close\r\n\r\n";
const char *pdf_header = "HTTP/1.1 200 OK\r\nContent-Type: application/pdf\r\nConnection: close\r\n\r\n";
const char *notfound_header = "HTTP/1.1 404 Not Found\r\n\r\n";

// Remove duplicate slashes
std::string normalize_slashes(const std::string& path) {
    std::string normalized;
    bool prev_slash = 0;
    for (char c : path) {
        if (c == '/') {
            if (!prev_slash) normalized += c;
            prev_slash = 1;
        } else {
            normalized += c;
            prev_slash = 0;
        }
    }
    return normalized;
}

// Returns the path of the file requested by HTTP, and the type of the file.
// (Uses C++ strings for easier manipulation)
std::pair<std::string, std::string> parse_http_request(std::string request) {
    std::stringstream ss(request);
    std::string word = "";  
    while (word != "GET") {
        ss >> word;
    }
    ss >> word;

    word = normalize_slashes(word);
    word = "website" + word;
    while (*word.rbegin() == '/') {
        word.erase(word.end() - 1);
    }
    if (word.find(".") == std::string::npos) {
        word += "/index.html";
    }
    
    return {word, word.substr(word.find(".") + 1)};
}

// Gets the content from path. Returns the length of the content.
int get_response(char *response, const char path[], std::string type) {
    if (type == "html") strcat(response, html_header);
    else if (type == "css") strcat(response, css_header);
    else if (type == "js") strcat(response, java_header);
    else if (type == "pdf") strcat(response, pdf_header);
    else if (type == "png") strcat(response, png_header);
    else {
        fprintf(stderr, "Unkown file extension\n");
        strcat(response, notfound_header);
        return strlen(response);
    }
    
    if (type == "html" || type == "css" || type == "js") {
        FILE *fp = fopen(path, "r");
        if (fp == NULL) {
            perror("fopen");
            strcat(response, notfound_header);
            return strlen(response);
        }

        char line[1000];
        while (fgets(line, 1000, fp) != NULL) {
            line[strlen(line) + 1] = '\0';
            line[strlen(line) - 1] = '\r';
            line[strlen(line)] = '\n';  // (Make sure last line ends with '\n')
            strcat(response, line);
        }

        if (fclose(fp)) {
            perror("fclose");
            exit(1);
        }
        return strlen(response);
    } else {
        FILE *fp = fopen(path, "rb");
        if (fp == NULL) {
            perror("fopen");
            strcat(response, notfound_header);
            return strlen(response);
        }
        
        char buffer[4096];
        int n, cur_len = strlen(response);

        while ((n = fread(buffer, sizeof(char), sizeof(buffer), fp)) != 0) {
            memcpy(response + cur_len, buffer, n);
            cur_len += n;
        }

        if (fclose(fp)) {
            perror("fclose");
            exit(1);
        }
        return cur_len;
    }
}

int main() {
    // Create socket
    int listen_soc = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_soc == -1) {
        perror("socket");
        exit(1);
    }

    // Bind socket
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);  // (host to network order)
    addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(addr.sin_zero), 0, 8);
    if (bind(listen_soc, (sockaddr *) &addr, sizeof(sockaddr_in))) {
        perror("bind");
        exit(1);
    }

    // Listen
    if (listen(listen_soc, 10) == -1) {
        perror("listen");
        exit(1);
    }

    // Accepting loop
    while (1) {
        std::cout << "Waiting for client...\n";

        sockaddr_in client_addr;
        socklen_t client_len = sizeof(sockaddr_in);
        int client_soc;
        client_addr.sin_family = AF_INET;

        if ((client_soc = accept(listen_soc, (sockaddr *) &client_addr, &client_len)) == -1) {
            perror("accept");
            exit(1);
        }

        std::cout << "Client connected\n";

        char buf[16000];  // (Should be enough for lightweight servers; Further improvemnts -> buffered reader)
        int n = read(client_soc, buf, sizeof(buf));
        buf[n] = '\0';
        std::pair<std::string, std::string> path_type = parse_http_request(buf);
        std::cout << "Requesting " << path_type.second << " file at " << path_type.first << "\n";
        
        char response[500000] = {'\0'};
        int len = get_response(response, path_type.first.c_str(), path_type.second);
        write(client_soc, response, len);
        close(client_soc);
    }
}
#include <iostream>
#include <cstdlib>
#include <string>
#include <sys/inotify.h>
#include <unistd.h>

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

// Function to shutdown a service
void shutdown_service(const std::string &service_name) {
    std::string command = "sudo systemctl stop " + service_name;
    int result = system(command.c_str());
    if(result == 0) {
        std::cout << service_name << " service stopped successfully.\n";
    } else {
        std::cerr << "Failed to stop " << service_name << " service.\n";
    }
}

// Function to handle the notification and shutdown process
void handle_sql_dump(const std::string &filename) {
    std::cout << "\033[41;37m"; // Set background to red and text to white
    std::cout << "\a\a\a"; // Beep three times
    std::cout << "ALERT! SQL dump detected: " << filename << "\033[0m" << std::endl; // Reset colors

    // Shutdown services
    shutdown_service("mariadb");
    shutdown_service("mysql");
    shutdown_service("oracle");
    // Add other SQL services if needed
}

// Function to monitor directory
void monitor_directory(const std::string &dir_to_watch) {
    int length, i = 0;
    int fd = inotify_init();
    if (fd < 0) {
        perror("inotify_init");
        exit(EXIT_FAILURE);
    }

    int wd = inotify_add_watch(fd, dir_to_watch.c_str(), IN_CREATE);
    if (wd == -1) {
        std::cerr << "Could not watch " << dir_to_watch << std::endl;
        exit(EXIT_FAILURE);
    }

    char buffer[EVENT_BUF_LEN];
    while (true) {
        length = read(fd, buffer, EVENT_BUF_LEN);
        if (length < 0) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        while (i < length) {
            struct inotify_event *event = (struct inotify_event *)&buffer[i];
            if (event->len) {
                if (event->mask & IN_CREATE) {
                    std::string filename(event->name);
                    if (filename.find(".sql") != std::string::npos) {
                        handle_sql_dump(filename);
                    }
                }
            }
            i += EVENT_SIZE + event->len;
        }
        i = 0;
    }

    inotify_rm_watch(fd, wd);
    close(fd);
}

int main() {
    std::string directory_to_watch = "/path/to/monitor"; // Change this to the directory where you expect SQL dumps to appear

    std::cout << "Monitoring directory: " << directory_to_watch << " for SQL dumps...\n";
    monitor_directory(directory_to_watch);

    return 0;
}

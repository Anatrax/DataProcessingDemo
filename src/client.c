/*
 * @brief Reads and parses data from Data Server
 * @author Samuel D. Villegas
 */
#include <ctype.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define BUF_SIZE 1400
#define SOCKET_EOL "@@"
#define SOCKET_EOF "END"
#define SOCKET_RESET "##"

struct TelemetryData {
  double time;
  double sim_time_step;
  double comp_time;
};

double parseDouble(char *c, char **ret_ptr) {
  double dbl;

  while (*c != ':') c++;  // Skip label
  c++;                    // Skip ':'

  // Null terminate the value so strtod will work
  int i = 0;
  while (*(c + i) != ';') i++;
  *(c + i) = '\0';

  // Turn string into double
  char *eptr;
  dbl = strtod(c, &eptr);
  if (dbl == 0) dbl = -1.0;  // errno == ERANGE

  c = eptr;  // Update c past value
  c++;       // Skip ';' (The one we turned into '\0')

  *ret_ptr = c;  // Set updated pointer
  return dbl;    // Return double
}

struct TelemetryData parseData(char *data_str) {
  struct TelemetryData data;

  char *eptr;
  data.time = parseDouble(data_str, &eptr);
  data.sim_time_step = parseDouble(eptr, &eptr);
  data.comp_time = parseDouble(eptr, &eptr);

  return data;
}

void check(const int return_val,
           const char *msg);  // Error function used for reporting issues
void checkConnectionError(const int return_val,
                          const char *port_str);  // Error function used for
                                                  // reporting connection issues
int isNumber(const char *str);  // Check if string is numerical
char *getResponse(int fd);      // Returns string from socket
struct sockaddr_in getPortAddress(char *host_str,
                                  char *port_str);  // Get socket address
int getServer(char *port_str);  // Get server's file descriptor

int main(int argc, char *argv[]) {
  // Check usage
  if (argc != 2) {
    fprintf(stderr, "USAGE: %s <port>\n", argv[0]);
    exit(1);
  }

  int server = getServer(argv[1]);
  while (1) {
    char *res = getResponse(server);  // Get response from server
    printf("-----\n>>>> \"%s\"\n", res);
    if (strstr(res, SOCKET_EOF) != NULL) break;
    struct TelemetryData data = parseData(res);
    printf("Time: %f,\nSimulation time step: %f,\nComputation time: %f\n",
           data.time, data.sim_time_step, data.comp_time);
  }

  close(server);
  return 0;
}

int getServer(char *port_str) {
  // Set up socket
  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);  // Create the socket
  checkConnectionError(socket_fd, port_str);

  // Connect to daemon
  struct sockaddr_in Addr =
      getPortAddress("localhost", port_str);  // Get port address
  int res = connect(socket_fd, (struct sockaddr *)&Addr,
                    sizeof(Addr));  // Connect socket to address
  checkConnectionError(res, port_str);

  // Set socket timeout to 5 seconds
  struct timeval tv;
  tv.tv_sec = 5;
  tv.tv_usec = 0;
  setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof tv);

  return socket_fd;
}

struct sockaddr_in getPortAddress(char *host_str, char *port_str) {
  struct sockaddr_in PortAddr;
  bzero(&PortAddr, sizeof(PortAddr));  // Clear address struct
  PortAddr.sin_family = AF_INET;       // Create a network - capable socket
  checkConnectionError(isNumber(port_str),
                       port_str);  // Check if port argument is numerical
  int port_num =
      atoi(port_str);  // Convert port number to an integer from a string
  PortAddr.sin_port = htons(port_num);  // Store port number
  struct hostent *HostInfo = gethostbyname(
      host_str);  // Convert hostname into a special form of address
  if (HostInfo == NULL) checkConnectionError(-1, port_str);
  memcpy((char *)&PortAddr.sin_addr.s_addr, (char *)HostInfo->h_addr,
         HostInfo->h_length);  // Copy address in
  return PortAddr;
}

char *getResponse(int fd) {
  // Set up response string
  int total_len = 0;
  int capacity = BUF_SIZE;
  char *str = calloc(capacity, sizeof(char));

  // Set up buffer
  char buffer[BUF_SIZE];
  int chars_read;
  char *msg_end;
  bzero(buffer, BUF_SIZE);  // Clear buffer

  while ((chars_read = read(fd, buffer, BUF_SIZE - 1)) > 0) {
    // Handle message termination string
    if ((msg_end = strstr(buffer, SOCKET_EOL))) {
      *msg_end = '\0';  // Remove message termination string from message
      strcat(str, buffer);
      break;
    }

    // Handle resending message string
    if ((msg_end = strstr(buffer, SOCKET_RESET))) {
      // Reset message string
      free(str);
      total_len = 0;
      capacity = BUF_SIZE;
      str = calloc(capacity, sizeof(char));

      // Send reset confirmation
      send(fd, SOCKET_RESET, strlen(SOCKET_RESET), 0);

      continue;
    }

    // Resize response string to fit longer messages
    if (chars_read + total_len > capacity) {
      capacity += 4 * BUF_SIZE;
      char *new_str = realloc(str, capacity * sizeof(char));
      if (new_str == NULL) check(-1, "ERROR reallocating response buffer\n");
      str = new_str;
    }

    // Append buffered string to response message
    strcat(str, buffer);
    total_len += chars_read;

    // Clear buffer for reuse
    bzero(buffer, BUF_SIZE);
  }
  check(chars_read, "ERROR reading from socket\n");
  return str;
}

int isNumber(const char *str) {
  int length = strlen(str);
  int i;
  for (i = 0; i < length; i++)
    if (!isdigit(str[i])) return -1;
  return 0;
}

void checkConnectionError(const int return_val, const char *port_str) {
  if (return_val < 0) {
    fprintf(stderr, "Error: could not contact server on port %s\n", port_str);
    exit(2);
  }
}

void check(const int return_val, const char *msg) {
  if (return_val < 0) {
    fprintf(stderr, msg);
    exit(1);
  }
}

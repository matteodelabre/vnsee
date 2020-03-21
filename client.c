#include "defs.h"
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int create_client_socket()
{
    int sck = socket(AF_INET, SOCK_DGRAM, 0);

    if (sck == -1)
    {
        perror("Unable to create client socket");
        exit(EXIT_FAILURE);
    }

    return sck;
}

struct sockaddr_in create_server_addr(short port)
{
    struct sockaddr_in addr = {0};
    inet_aton("10.11.99.1", &addr.sin_addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    return addr;
}

void send_line(int sck, struct sockaddr_in addr, const line_update* message)
{
    ssize_t bytes_sent = sendto(
        sck, message, sizeof(line_update),
        /* flags = */ 0,
        (struct sockaddr*) &addr, sizeof(addr)
    );

    if (bytes_sent == -1)
    {
        perror("Could not send line");
        exit(EXIT_FAILURE);
    }

    if (bytes_sent < sizeof(line_update))
    {
        fprintf(stderr, "Line sent partially\n");
        exit(EXIT_FAILURE);
    }
}

void send_flush(int sck, struct sockaddr_in addr)
{
    line_update message;
    message.line_idx = SCREEN_ROWS;
    send_line(sck, addr, &message);
}

int main(int argc, char** argv)
{
    int sck = create_client_socket();
    struct sockaddr_in server_addr = create_server_addr(/* port = */ 7777);

    line_update message;
    message.line_idx = 0;

    int img = open("./image.pbm", O_RDONLY);
    size_t line_col = 0;

    char image_buffer[2048];
    ssize_t bytes_read;

    while ((bytes_read = read(img, image_buffer, sizeof(image_buffer))) > 0)
    {
        for (size_t i = 0; i < bytes_read; ++i)
        {
            if (image_buffer[i] == '0' || image_buffer[i] == '1')
            {
                if (argc == 1)
                {
                    message.buffer[line_col] =
                        image_buffer[i] == '0' ? 0xffff : 0;
                }
                else
                {
                    message.buffer[line_col] = 0xffff;
                }

                ++line_col;

                if (line_col == SCREEN_COLS)
                {
                    line_col = 0;
                    send_line(sck, server_addr, &message);
                    ++message.line_idx;
                }
            }
        }
    }

    send_flush(sck, server_addr);
    close(img);
    close(sck);
    return EXIT_SUCCESS;
}

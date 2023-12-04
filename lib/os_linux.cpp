
#include <fcntl.h>
#include <poll.h>
#include <sys/file.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>

namespace Os_codes {
    
    enum Open_flags: u64 {
        OPEN_READ  = 1,
        OPEN_WRITE = 2,
        OPEN_CREATE = 4,
        OPEN_APPEND = 8,
        OPEN_TRUNCATE = 16,
        OPEN_DIRECTORY = 32,
    };
    
    enum General_code: s64 {
        SUCCESS = 0,
        ERROR = 1,
        SKIPPED = -1, // Skipped due to existing error code
    };
    
    enum Write_all_code: s64 {
        WRITE_ERROR = 101,
        WRITE_EOF = 102,
        WRITE_WOULDBLOCK = 103
    };
    
    enum Read_all_code: s64 {
        READ_ERROR = 201,
        READ_EOF = 202,
        READ_WOULDBLOCK = 203
    };
    
    enum Seek_whence: u8 {
        P_SEEK_SET,
        P_SEEK_CUR,
        P_SEEK_END
    };
    
    enum Access_flags: u8 {
        ACCESS_EXISTS = 1,
        ACCESS_READ = 2,
        ACCESS_WRITE = 4,
        ACCESS_EXECUTE = 8
    };
    
};

struct Status {
    s64 code = 0;
    Array_dyn<u8> error_buf;
    
    bool good() const { return code == 0; }
    bool bad()  const { return code != 0; }
};

// Only necessary to call if an error occurred
void os_status_free(Status* status) {
    array_free(&status->error_buf);
}

struct Os_data {
    bool initialized;
    timespec t_start; // Time of program start
    Status status;
};

Os_data global_os;

void os_init() {
    clock_gettime(CLOCK_MONOTONIC_RAW, &global_os.t_start);
    global_os.initialized = true;
}

u64 os_now() {
    assert(global_os.initialized);
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (ts.tv_sec - global_os.t_start.tv_sec) * 1000000000ull
        + (ts.tv_nsec - global_os.t_start.tv_nsec);
}

bool os_status_initp(Status** statusp) {
    if (*statusp == nullptr) *statusp = &global_os.status;
    return (**statusp).bad();
}

template <typename... T>
void os_error_printf(Status* status, s64 code, char const* fmt, T... args) {
    assert(fmt);
    status->code = code;
    if (fmt && fmt[0] == '$') {
        array_printf(&status->error_buf, "%s\n", strerror(errno));
        ++fmt;
        fmt += fmt[0] == ' ';
    }
    array_printf(&status->error_buf, fmt, args...);
    if (status->error_buf.size and status->error_buf.back() != '\n') {
        array_push_back(&status->error_buf, '\n');
    }
}

int os_open(Array_t<u8> path, u64 flags, u32 mode=0, Status* status = nullptr) {
    if (not status) status = &global_os.status;
    if (status->code) return -1;
    
    using namespace Os_codes;
    assert(not (flags & ~(OPEN_READ | OPEN_WRITE | OPEN_CREATE | OPEN_APPEND | OPEN_TRUNCATE | OPEN_DIRECTORY)));
    
    char* tmp = (char*)alloca(path.size + 1);
    memcpy(tmp, path.data, path.size);
    tmp[path.size] = 0;
    
    int f = 0;
    u64 flagsrw = flags & (OPEN_READ | OPEN_WRITE);
    f |= flagsrw == (OPEN_READ | OPEN_WRITE) ? O_RDWR : 0;
    f |= flagsrw == OPEN_READ ? O_RDONLY : 0;
    f |= flagsrw == OPEN_WRITE ? O_WRONLY : 0;
    assert(flagsrw);
    
    f |= flags & OPEN_CREATE ? O_CREAT : 0;
    f |= flags & OPEN_APPEND ? O_APPEND : 0;
    f |= flags & OPEN_TRUNCATE ? O_TRUNC : 0;
    f |= flags & OPEN_DIRECTORY ? O_DIRECTORY : 0;
    
    int fd = open(tmp, f, (mode_t)mode);
    
    if (fd == -1) {
        os_error_printf(status, ERROR, "$ while opening file '%s'\n", tmp);
        return -1;
    }
    
    return fd;
}
void os_close(int fd, Status* status = nullptr) {
    if (os_status_initp(&status)) return;
    int code = close(fd);
    if (code) os_error_printf(status, 221206101, "$ while calling close()\n");
}

bool os_read(int fd, Array_dyn<u8>* buf, s64 n, Status* status = nullptr) {
    if (os_status_initp(&status)) return false;
    
    array_reserve(buf, buf->size + n);
    
    s64 bytes_read = read(fd, buf->data + buf->size, n);
    if (bytes_read == -1) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            status->code = Os_codes::READ_WOULDBLOCK;
            return false;
        }
        os_error_printf(status, Os_codes::READ_ERROR, "$ while calling read()\n");
        return false;
    }
    buf->size += bytes_read;
    
    return bytes_read == 0; // return eof
}

void os_read_fixed(int fd, Array_t<u8> buf, Status* status = nullptr) {
    if (os_status_initp(&status)) return;
    
    while (buf.size > 0) {
        s64 bytes_read = read(fd, buf.data, buf.size);
        if (bytes_read == -1) {
            if (errno == EWOULDBLOCK or errno == EAGAIN) {
                status->code = Os_codes::READ_WOULDBLOCK;
                return;
            }
            return os_error_printf(status, Os_codes::READ_ERROR, "$ while calling read()\n");
        }
        if (bytes_read == 0) {
            /* Note that buf.size > 0 due to loop condition */
            return os_error_printf(status, Os_codes::READ_EOF, "unexpected eof (%ld bytes left to read)\n", (long)buf.size);
        }
        buf = array_subarray(buf, bytes_read, buf.size);
    }
}

s64 os_write_fixed(int fd, Array_t<u8> buf, Status* status = nullptr) {
    if (os_status_initp(&status)) return 0;
    
    s64 total = 0;
    while (buf.size > 0) {
        s64 bytes_written = write(fd, buf.data, buf.size);
        if (bytes_written == -1) {
            if (errno == EPIPE) {
                os_error_printf(status, Os_codes::WRITE_EOF, "eof while writing bytes (%ld left to write)\n", (long)buf.size);
                return total;
            } else {
                if (errno == EWOULDBLOCK or errno == EAGAIN) {
                    status->code = Os_codes::WRITE_WOULDBLOCK;
                    return total;
                }
                os_error_printf(status, Os_codes::WRITE_ERROR, "$ while calling write()\n");
                return total;
            }
        }
        assert(bytes_written > 0);
        total += bytes_written;
        buf = array_subarray(buf, bytes_written, buf.size);
    }
    return total;
}


s64 os_seek(int fd, u64 offset, u8 whence, Status* status = nullptr) {
    if (os_status_initp(&status)) return 0;
    
    int w = SEEK_SET;
    switch (whence) {
        case Os_codes::P_SEEK_SET: w = SEEK_SET; break;
        case Os_codes::P_SEEK_CUR: w = SEEK_CUR; break;
        case Os_codes::P_SEEK_END: w = SEEK_END; break;
        default: assert_false;
    }
    
    off_t r = lseek(fd, offset, w);
    if (r == (off_t)-1) {
        os_error_printf(status, 221206001, "$ while calling lseek()\n");
        return -1;
    }
    s64 rr = r;
    if (rr != r) {
        os_error_printf(status, 221206002, "seek offset does not fit into an s64\n");
        return -1;
    }
    
    return r;
}

Array_t<u8> os_read_file(Array_t<u8> path, Status* status = nullptr) {
    if (os_status_initp(&status)) return {};
    
    int fd = os_open(path, Os_codes::OPEN_READ, 0, status);
    s64 length = os_seek(fd, 0, Os_codes::P_SEEK_END, status);
    os_seek(fd, 0, Os_codes::P_SEEK_SET, status);
    
    if (status->bad()) return {};
    
    Array_t<u8> result = array_create<u8>((s64)length);
    os_read_fixed(fd, result, status);
    os_close(fd, status);
    
    return result;
}
void os_write_file(Array_t<u8> path, Array_t<u8> data, Status* status = nullptr) {
    if (os_status_initp(&status)) return;
    
    int fd = os_open(path, Os_codes::OPEN_WRITE | Os_codes::OPEN_CREATE | Os_codes::OPEN_TRUNCATE, 0660, status);
    os_write_fixed(fd, data, status);
    os_close(fd, status);
}

bool os_access(Array_t<u8> path, u8 flags) {
    using namespace Os_codes;
    int mode = 0;
    mode |= flags & ACCESS_EXISTS  ? F_OK : 0;
    mode |= flags & ACCESS_READ    ? R_OK : 0;
    mode |= flags & ACCESS_WRITE   ? W_OK : 0;
    mode |= flags & ACCESS_EXECUTE ? X_OK : 0;
    
    char* tmp = (char*)alloca(path.size + 1);
    memcpy(tmp, path.data, path.size);
    tmp[path.size] = 0;
    
    return access(tmp, mode) != -1;
}

void os_error_output(Array_t<u8> prefix = "Error"_arr, Status* status = nullptr) {
    if (not status) status = &global_os.status;
    
    Status temp_status;
    s64 last = 0;
    for (s64 i = 0; i < status->error_buf.size; ++i) {
        if (status->error_buf[i] == '\n') {
            os_write_fixed(STDERR_FILENO, prefix, &temp_status);
            os_write_fixed(STDERR_FILENO, ": "_arr, &temp_status);
            os_write_fixed(STDERR_FILENO, array_subarray(status->error_buf, last, i), &temp_status);
            os_write_fixed(STDERR_FILENO, "\n"_arr, &temp_status);
            last = i+1;
        }
    }
    status->error_buf.size = 0;
    
    if (temp_status.bad()) {
        // Encountering an error while trying to print an error message is not a good sign
        abort();
    }
}

void os_error_panic(Status* status = nullptr) {
    if (not status) status = &global_os.status;
    if (status->bad()) {
        os_error_output("Error"_arr, status);
        exit(2);
    }
}

bool os_error_clear(Status* status = nullptr) {
    if (not status) status = &global_os.status;
    status->error_buf.size = 0;
    bool result = status->code;
    status->code = 0;
    return result;
}

Array_t<u8> array_load_from_file(Array_t<u8> path) {
    Array_t<u8> result = os_read_file(path);
    os_error_panic();
    return result;
}
void array_write_to_file(Array_t<u8> path, Array_t<u8> data) {
    os_write_file(path, data);
    os_error_panic();
}
void array_fwrite(Array_t<u8> str, int fd = STDOUT_FILENO) {
    os_write_fixed(fd, str);
    os_error_panic();
}
void array_puts(Array_t<u8> str, int fd = STDOUT_FILENO) {
    os_write_fixed(fd, str);
    os_write_fixed(fd, "\n"_arr);
    os_error_panic();
}

struct Dir_entries {
    enum Types: u8 {
        INVALID, BLOCK_DEVICE, CHARACTER_DEVICE, DIRECTORY, NAMED_PIPE, SYMBOLIC_LINK, UNIX_SOCKET, FILE
    };
    struct Entry {
        Offset<u8> name;
        u8 type = 0;
    };
    Array_dyn<Entry> entries;
    Array_dyn<u8> name_data;
    Array_dyn<u8> _buf;
    
    Array_t<u8> get_name(s64 i) { return array_suboffset(name_data, entries[i].name); }
};
void os_dir_entries_free(Dir_entries* entries) {
    array_free(&entries->entries);
    array_free(&entries->name_data);
    array_free(&entries->_buf);;
}

void os_directory_list(Array_t<u8> path, Dir_entries* out_entries, Status* status=nullptr) {
    struct linux_dirent64 {
        ino64_t        d_ino;    /* 64-bit inode number */
        off64_t        d_off;    /* 64-bit offset to next structure */
        unsigned short d_reclen; /* Size of this dirent */
        unsigned char  d_type;   /* File type */
        char           d_name[]; /* Filename (null-terminated) */
    };
    
    if (os_status_initp(&status)) return;
    
    int fd = os_open(path, Os_codes::OPEN_READ | Os_codes::OPEN_DIRECTORY, 0, status);
    if (status->bad()) return;
    
    array_reserve(&out_entries->_buf, 32768);
    out_entries->_buf.size = 0;
    
    while (true) {
        s64 n = getdents64(fd, out_entries->_buf.data, out_entries->_buf.capacity);
        if (n == -1) return os_error_printf(status, 20231104001, "$ while calling getdents64()\n");
        out_entries->_buf.size = n;
        if (n == 0) break;
        
        for (s64 i = 0; i < out_entries->_buf.size;) {
            linux_dirent64* ent = (linux_dirent64*)&out_entries->_buf[i];
            Dir_entries::Entry entry;
            entry.name = array_append(&out_entries->name_data, array_create_str(ent->d_name));
            
            switch (ent->d_type) {
                case DT_BLK:  entry.type = Dir_entries::BLOCK_DEVICE; break;
                case DT_CHR:  entry.type = Dir_entries::CHARACTER_DEVICE; break;
                case DT_DIR:  entry.type = Dir_entries::DIRECTORY; break;
                case DT_FIFO: entry.type = Dir_entries::NAMED_PIPE; break;
                case DT_LNK:  entry.type = Dir_entries::SYMBOLIC_LINK; break;
                case DT_REG:  entry.type = Dir_entries::FILE; break;
                case DT_SOCK: entry.type = Dir_entries::UNIX_SOCKET; break;
                case DT_UNKNOWN: entry.type = Dir_entries::INVALID; break;
                default: return os_error_printf(status, 20231104002, "unknown file type %d\n", (int)ent->d_type);
            }
            
            array_push_back(&out_entries->entries, entry);
            
            i += ent->d_reclen;
        }
    }
    
    for (auto& i: out_entries->entries) {
        if (i.type != Dir_entries::INVALID) continue;
        
        out_entries->_buf.size = 0;
        array_printa(&out_entries->_buf, "%/%", path, array_suboffset(out_entries->name_data, i.name));
        array_push_back(&out_entries->_buf, 0);
        
        struct stat st;
        int code = lstat((char*)out_entries->_buf.data, &st);
        if (code) return os_error_printf(status, 20231104003, "while calling lstat() on %s", (char*)out_entries->_buf.data);
        
        switch (st.st_mode & S_IFMT) {
            case S_IFSOCK: i.type = Dir_entries::UNIX_SOCKET; break;
            case S_IFLNK:  i.type = Dir_entries::SYMBOLIC_LINK; break;
            case S_IFREG:  i.type = Dir_entries::FILE; break;
            case S_IFBLK:  i.type = Dir_entries::BLOCK_DEVICE; break;
            case S_IFDIR:  i.type = Dir_entries::DIRECTORY; break;
            case S_IFCHR:  i.type = Dir_entries::CHARACTER_DEVICE; break;
            case S_IFIFO:  i.type = Dir_entries::NAMED_PIPE; break;
            default: return os_error_printf(status, 20231104004, "unknown file type %x (from lstat)\n", (int)st.st_mode & S_IFMT);
        }
    }
}

#define OS_PAD_ZERO(x, y) \
char* x; \
if (((u64)(y.data + y.size) % 4096) and y.data[y.size] == 0) { \
x = (char*)y.data; \
} else { \
x = (char*)alloca(y.size + 1); \
memcpy(x, y.data, y.size); \
x[y.size] = 0; \
}

s64 os_get_file_size(Array_t<u8> path, Status* status=nullptr) {
    if (os_status_initp(&status)) return -1;
    
    OS_PAD_ZERO(path_c, path);
    struct stat s;
    int code = stat(path_c, &s);
    if (code == -1) {
        os_error_printf(status, 20231126005, "$ while calling stat on %s\n", path_c);
        return -1;
    }
    
    return s.st_size;
}

void os_unlink(Array_t<u8> path, Status* status=nullptr) {
    if (os_status_initp(&status)) return;
    OS_PAD_ZERO(path_c, path);
    int code = unlink(path_c);
    if (code == -1) return os_error_printf(status, 20231126005, "$ while calling unlink on %s\n", path_c);
}

void os_rename(Array_t<u8> from, Array_t<u8> to, Status* status=nullptr) {
    if (os_status_initp(&status)) return;
    OS_PAD_ZERO(from_c, from);
    OS_PAD_ZERO(to_c, to);
    int code = rename(from_c, to_c);
    if (code == -1) return os_error_printf(status, 20231130001, "$ while doing rename %s -> %s\n", from_c, to_c);
}

struct Os_process_info {
    enum Flags: u8 {
        STDIN_INHERIT = 1,
        STDOUT_INHERIT = 2,
        STDERR_INHERIT = 4,
    };
    // Options
    u8 flags = 0;
    Array_dyn<s64> args_indices;
    Array_dyn<u8> args_data;
    Status* status = nullptr;
    
    // State
    struct Fd {
        int fd_old;
        bool input;
        int pipe[2] = {};
    };
    pid_t pid = 0;
    Array_dyn<Fd> fds;
    Array_dyn<struct pollfd> fds_poll;
    Array_dyn<u8> stdin_buf;
    Array_dyn<u8> stdout_buf;
    Array_dyn<u8> stderr_buf;
    bool stdin_eof = false;
    
    // Termination information
    bool exited = false;
    bool aborted = false;
    u8 exit_code = 0;
    
    bool done() { return (status and status->bad()) or (exited and fds.size == 0); }
};

void os_process_free(Os_process_info* info) {
    array_free(&info->args_indices);
    array_free(&info->args_data);
    array_free(&info->fds);
    array_free(&info->fds_poll);
    array_free(&info->stdin_buf);
    array_free(&info->stdout_buf);
    array_free(&info->stderr_buf);
}

void os_process_create(Os_process_info* info) {
    if (os_status_initp(&info->status)) return;
    
    info->fds.size = 0;
    info->exited = false;
    info->aborted = false;
    
    array_append_zero(&info->args_data, info->args_indices.size-1);
    for (s64 i = info->args_indices.size-1; i > 0; --i) {
        s64 beg = info->args_indices[i-1];
        s64 end = info->args_indices[i];
        memmove(&info->args_data[beg+i-1], &info->args_data[beg], end - beg);
        info->args_data[end+i-1] = 0;
        info->args_indices[i] += i;
    }
    Array_dyn<char*> argv;
    for (s64 i = 0; i < info->args_indices.size-1; ++i) {
        array_push_back(&argv, (char*)&info->args_data[info->args_indices[i]]);
    }
    array_push_back(&argv, nullptr);
    
    if ((~info->flags & Os_process_info::STDIN_INHERIT )) array_push_back(&info->fds, {STDIN_FILENO , true});
    if ((~info->flags & Os_process_info::STDOUT_INHERIT)) array_push_back(&info->fds, {STDOUT_FILENO, false});
    if ((~info->flags & Os_process_info::STDERR_INHERIT)) array_push_back(&info->fds, {STDERR_FILENO, false});
    
    for (auto& i: info->fds) {
        if (pipe(i.pipe)) goto error;
        if (i.input) simple_swap(&i.pipe[0], &i.pipe[1]);
    }
    
    {pid_t parent_pid = getpid();
        
        info->pid = fork();
        if (info->pid == 0) {
            // Die when the parent dies (the getppid() check avoids a race condition)
            if (prctl(PR_SET_PDEATHSIG, SIGTERM) == -1) goto error2;
            if (getppid() != parent_pid) goto error2;
            
            for (auto& i: info->fds) {
                if (dup2(i.pipe[1], i.fd_old) == -1) goto error2;
                os_close(i.pipe[0], info->status);
            }
            
            if (info->status->bad()) goto error2;
            
            execvp(argv[0], argv.data);
            goto error2;
        } else if (info->pid != -1) {
            for (auto& i: info->fds) {
                array_push_back(&info->fds_poll, (struct pollfd) {i.pipe[0], (short)(i.input ? POLLOUT : POLLIN), 0});
                if (fcntl(i.pipe[0], F_SETFL, O_NONBLOCK) != 0) goto error2;
                os_close(i.pipe[1], info->status);
                i.pipe[1] = -1;
            }
        } else {
            goto error;
        }
    }
    
    array_free(&argv);
    return;
    
    error:
    return os_error_printf(info->status, 231015001, "$ while initialising child\n");
    error2:
    os_error_printf(info->status, 231015002, "$ while initialising child\n");
    os_error_panic(info->status);
}

Array_t<struct pollfd> os_process_communicate_prepare(Os_process_info* info) {
    for (s64 i = 0; i < info->fds.size; ++i) {
        if (info->fds[i].input) {
            info->fds_poll[i].events = info->stdin_buf.size ? POLLOUT : 0;
        }
    }
    return info->fds_poll;
}
void os_process_communicate_process(Os_process_info* info, Array_t<struct pollfd> fds_poll) {
    for (s64 i_it = 0; i_it < info->fds.size; ++i_it) {
        auto i = fds_poll[i_it];
        auto ii = info->fds[i_it];
        if (i.revents & POLLERR) {
            if (ii.input and i.events == 0) i.revents = POLLHUP;
            else return os_error_printf(info->status, 231015004, "POLLERR during poll\n");
        }
        Array_dyn<u8>* buf = (ii.fd_old == STDIN_FILENO ? &info->stdin_buf :
                              ii.fd_old == STDOUT_FILENO ? &info->stdout_buf : &info->stderr_buf);
        
        if (ii.input and (i.revents & POLLHUP)) {
            if (buf->size) return os_error_printf(info->status, 231015005, "pipe closed with data left to write (%lld bytes left)\n", buf->size);
            info->fds[i_it].fd_old = -1;
            
        } else if (ii.input and (i.revents & POLLOUT)) {
            if (info->status->bad()) continue;
            s64 written = os_write_fixed(i.fd, *buf, info->status);
            if (info->status->code == Os_codes::WRITE_WOULDBLOCK) info->status->code = 0;
            array_pop_front(&info->stdin_buf, written);
            
        } else if (not ii.input and (i.revents & (POLLIN | POLLHUP))) {
            if (info->status->bad()) continue;
            bool eof = os_read(i.fd, buf, 4096, info->status);
            if (info->status->code == Os_codes::READ_WOULDBLOCK) info->status->code = 0;
            if (eof) {
                info->fds[i_it].fd_old = -1;
            }
        }
        
        if (ii.input and info->stdin_eof and buf->size == 0) {
            info->fds[i_it].fd_old = -1;
        }
    }
    for (s64 i = 0; i < info->fds.size; ++i) {
        if (info->fds[i].fd_old != -1) continue;
        if (info->status->bad()) return;
        os_close(info->fds_poll[i].fd, info->status);
        info->fds[i] = info->fds.back();
        info->fds_poll[i] = info->fds_poll.back();
        --info->fds.size;
        --info->fds_poll.size;
        --i;
    }
    
    if (not info->exited) {
        siginfo_t si;
        si.si_pid = 0;
        int mask = info->fds_poll.size ? WNOHANG : 0;
        int code = waitid(P_PID, info->pid, &si, WEXITED | mask);
        if (code == -1) return os_error_printf(info->status, 231015006, "$ while calling waitid\n");
        if (si.si_pid != 0) {
            info->exited = true;
            info->exit_code = si.si_status;
            info->aborted = si.si_code != CLD_EXITED;
        }
    }
}
void os_process_communicate(Os_process_info* info, int timeout=-1) {
    os_process_communicate_prepare(info);
    
    if (info->fds_poll.size) {
        int code = poll(info->fds_poll.data, info->fds_poll.size, timeout);
        if (code == -1) return os_error_printf(info->status, 231015003, "$ while calling poll\n");
    }
    
    os_process_communicate_process(info, info->fds_poll);
}

void os_process_close(Os_process_info* info) {
    if (info->aborted) {
        return os_error_printf(info->status, 231015007, "child aborted (code %d)\n", info->exit_code);
    }
    for (auto i: info->fds_poll) {
        if (info->status->bad()) break;
        os_close(i.fd, info->status);
        i.fd = -1;
    }
}

void os_process_run(Os_process_info* info) {
    os_process_create(info);
    while (not info->done()) {
        os_process_communicate(info, -1);
    }
    os_process_close(info);
}

void os_readlink(Array_t<u8> path, Array_dyn<u8>* out, Status* status) {
    if (os_status_initp(&status)) return;
    OS_PAD_ZERO(tmp, path);
    
    s64 cap_needed = 1;
    while (true) {
        array_reserve(out, out->size + cap_needed);
        s64 n = readlink(tmp, (char*)out->data + out->size, out->capacity - out->size);
        if (n == -1) {
            return os_error_printf(status, 20231105001, "$ while calling readlink() on %s\n", tmp);
        }
        if (n < out->capacity - out->size) {
            out->size += n;
            break;
        }
        cap_needed = cap_needed <= 1 ? 256 : 2*cap_needed;
    }
}

void os_chdir(Array_t<u8> path, Status* status=nullptr) {
    if (os_status_initp(&status)) return;
    OS_PAD_ZERO(tmp, path);
    
    int code = chdir(tmp);
    if (code == -1) {
        return os_error_printf(status, 20231105002, "$ while calling chdir() on %s\n", tmp);
    }
}

void os_chdir_exe(Status* status=nullptr) {
    if (os_status_initp(&status)) return;
    
    Array_dyn<u8> buf;
    defer { array_free(&buf); };
    
    os_readlink("/proc/self/exe"_arr, &buf, status);
    if (status->bad()) return;
    
    while (buf.size and buf.back() != '/') --buf.size;
    
    if (not buf.size) {
        return os_error_printf(status, 20231105003, "non-absolute link location\n");
    }
    
    os_chdir(buf, status);
}

void os_pipe(Array_t<int> pipes, Status* status=nullptr) {
    if (os_status_initp(&status)) return;
    assert(pipes.size == 2);
    int code = pipe(pipes.data);
    if (code) return os_error_printf(status, 20231125001, "$ while calling pipe()\n");
}

void os_dup(int oldfd, int newfd, Status* status=nullptr) {
    if (os_status_initp(&status)) return;
    int code = dup2(oldfd, newfd);
    if (code == -1) return os_error_printf(status, 20231130002, "$ while calling dup2()\n");
}

s32 os_fork(Status* status=nullptr) {
    if (os_status_initp(&status)) return -1;
    
    pid_t parent_pid = getpid();
    s32 p = fork();
    if (p == -1) {
        os_error_printf(status, 20231125002, "$ while calling fork()\n");
        return -1;
    }
    
    if (p == 0) {
        // Die when the parent dies (the getppid() check avoids a race condition)
        if (prctl(PR_SET_PDEATHSIG, SIGTERM) == -1) {
            os_error_printf(status, 20231125003, "$ while calling prctl()\n");
            return -1;
        }
        if (getppid() != parent_pid) abort();
    }
    
    return p;
}

void os_execvp(Array_t<u8*> args, Status* status=nullptr) {
    if (os_status_initp(&status)) return;
    execvp((char*)args[0], (char**)args.data);
    os_error_printf(status, 20231126001, "$ while executing %s\n", (char const*)args[0]);
}

struct Os_pipeline_info {
    // Options
    Array_dyn<s64> args_cmd_indices;
    Array_dyn<s64> args_indices;
    Array_dyn<u8> args_data;
    Status* status = nullptr;
    int stdin_fd  = -1; // will be closed
    int stdout_fd = -1; // will be closed
    
    void arg(Array_t<u8> s) {
        array_append(&args_data, s);
        array_push_back(&args_data, 0);
        array_push_back(&args_indices, args_data.size);
    }
    void cmd(std::initializer_list<Array_t<u8>> args) {
        if (args_indices.size == 0) array_push_back(&args_indices, 0);
        if (args_cmd_indices.size == 0) array_push_back(&args_cmd_indices, 0);
        for (auto s: args) arg(s);
        array_push_back(&args_cmd_indices, args_indices.size-1);
    }
};

void os_pipeline_free(Os_pipeline_info* info) {
    array_free(&info->args_cmd_indices);
    array_free(&info->args_indices);
    array_free(&info->args_data);
}

void os_pipeline(Os_pipeline_info* info) {
    if (os_status_initp(&info->status)) return;
    
    s64 n_procs = info->args_cmd_indices.size-1;
    
    Array_dyn<u8*> temp_args;
    defer { array_free(&temp_args); };
    Array_dyn<s32> temp_pids;
    defer { array_free(&temp_pids); };
    
    int next_stdin = info->stdin_fd;
    for (s64 i = 0; i < n_procs; ++i) {
        int this_stdin = next_stdin;
        int this_stdout;
        if (i+1 < n_procs) {
            int pipe[2];
            os_pipe({pipe, 2}, info->status);
            this_stdout = pipe[1];
            next_stdin = pipe[0];
        } else {
            this_stdout = info->stdout_fd;
            next_stdin = -1;
        }
        
        temp_args.size = 0;
        for (s64 arg: array_subindex(info->args_cmd_indices, info->args_indices, i)) {
            array_push_back(&temp_args, &info->args_data[arg]);
        }
        array_push_back(&temp_args, nullptr);
        
        s32 child_pid = os_fork(info->status);
        
        if (child_pid == 0) {
            if (next_stdin != -1) os_close(next_stdin, info->status);
            if (this_stdin != -1) os_dup(this_stdin, STDIN_FILENO, info->status);
            if (this_stdout != -1) os_dup(this_stdout, STDOUT_FILENO, info->status);
            os_execvp(temp_args, info->status);
            os_error_panic(info->status);
        }
        
        if (this_stdin != -1) os_close(this_stdin, info->status);
        if (this_stdout != -1) os_close(this_stdout, info->status);
        
        if (info->status->bad()) break;
        array_push_back(&temp_pids, child_pid);
    }
    
    for (s64 i = 0; i < n_procs; ++i) {
        int code;
        waitpid(temp_pids[i], &code, 0);
        
        if (!WIFEXITED(code)) return os_error_printf(info->status, 20231126002, "child %lld did not exit cleanly\n", i);
        if (WEXITSTATUS(code)) return os_error_printf(info->status, 20231126003, "child %lld exited with code %d\n", i, WEXITSTATUS(code));
    }
}

struct Os_execute_result {
    int exited = 0; // 1 if the process exited normally, 0 if abnormally, and -1 on error conditions
    int exit_code = -1;
};
Os_execute_result os_execute(Array_t<Array_t<u8>> args, Status* status=nullptr) {
    Os_execute_result result;
    if (os_status_initp(&status)) return result;
    
    Array_t<u8*> args_c = array_create<u8*>(args.size+1);
    Array_dyn<u8> temp;
    defer { array_free(&temp); };
    
    s64 size = 0;
    for (s64 i = 0; i < args.size; ++i) size += args[i].size + 1;
    array_reserve(&temp, size);
    temp.do_not_reallocate = true;
    
    for (s64 i = 0; i < args.size; ++i) {
        s64 index = temp.size;
        array_append(&temp, args[i]);
        array_push_back(&temp, 0);
        args_c[i] = &temp[index];
    }
    
    temp.do_not_reallocate = false;
    
    s32 child = os_fork();
    if (child == 0) {
        os_execvp(args_c);
        os_error_panic();
    }
    
    if (status->bad()) return result;
    
    int info;
    int code = waitpid(child, &info, 0);
    if (code == -1) {
        os_error_printf(status, 20231126004, "$ while calling waitpid\n");
        return result;
    }
    result.exited = WIFEXITED(info);
    result.exit_code = WEXITSTATUS(info);
    return result;
}

void os_execute_check(Array_t<Array_t<u8>> args, Status* status=nullptr) {
    if (os_status_initp(&status)) return;
    auto result = os_execute(args, status);
    if (status->bad()) return;
    if (result.exited != 1) {
        status->code = 20231201001;
        array_printa(&status->error_buf, "process aborted\nwhile executing %\n", args[0]);
    } else if (result.exit_code != 0) {
        status->code = 20231201002;
        array_printf(&status->error_buf, "process exited with code %d\n", result.exit_code);
        array_printa(&status->error_buf, "while executing %a\n", args[0]);
    }
}

void os_getcwd(Array_dyn<u8>* into, Status* status=nullptr) {
    // !!! UNTESTED !!!
    if (os_status_initp(&status)) return;
    s64 space = 256;
    while (true) {
        array_reserve(into, into->size + space);
        char* p = getcwd((char*)into->data+into->size, into->capacity-into->size);
        if (p == nullptr and errno == ERANGE) {
            space *= 2;
        } else if (not p) {
            return os_error_printf(status, 20231201003, "$ while calling getcwd()\n");
        } else break;
    }
    into->size += strlen((char*)(into->data + into->size));
}



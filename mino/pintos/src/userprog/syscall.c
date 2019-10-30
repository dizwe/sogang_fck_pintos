#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler(struct intr_frame*);
void halt(void);
void exit(int status);
// lib/user/syscall.h에pid_t 저장되어있음
pid_t exec(const char* cmd_lines);
int wait(pid_t pid);
bool create(const char* file, unsigned initial_size);
bool remove(const char* file);
int open(const char* file);
int filesize(int fd);
int read(int fd, void* buffer, unsigned size);
int write(int fd, const void* buffer, unsigned size);
void seek(int fd, unsigned position);
unsigned tell(int fd);
void close(int fd);
void check_address(void* addr);
#define WORD sizeof(uint32_t)
#define STDIN 0
#define STDOUT 1
#define STDERR 2

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

//####
//####
static void
syscall_handler(struct intr_frame* f UNUSED)
{


	uint32_t* args = f->esp;
	printf("--- arg num : %d", args[0]);
	//  printf ("system call!\n");
	//  printf ("num : %d\n",*(uint32_t*)(f->esp));
	switch (*(uint32_t*)(f->esp)) {
	case SYS_HALT:                   /* Halt the operating system. */
		halt();
		break;
	case SYS_EXIT:					/* Terminate this process. */
		check_address(f->esp + WORD);
		//exit((int) * (uint32_t*)(f->esp + WORD));
		//printf("exit num : %d",(int)args[1]);
		exit((int)args[1]);
		break;
	case SYS_EXEC:                   /* Start another process. */
		check_address(f->esp + WORD);
		exec((const char*) * (uint32_t*)(f->esp + WORD));
		break;
	case SYS_WAIT:                   /* Wait for a child process to die. */
		check_address(f->esp + WORD);
		wait((pid_t) * (uint32_t*)(f->esp + WORD));
		break;
	case SYS_CREATE:                 /* Create a file. */
		break;
	case SYS_REMOVE:                 /* Delete a file. */
		break;
	case SYS_OPEN:                   /* Open a file. */
		break;
	case SYS_FILESIZE:               /* Obtain a file's size. */
		break;
	case SYS_READ:                   /* Read from a file. */
		break;
	case SYS_WRITE:                  /* Write to a file. */
		//check_address(f->esp); 
  //	  printf("%d %d --data \n",args[1],args[3]);
		write((int)args[1], (void*)args[2], (unsigned)args[3]);
		// write((int) * (uint32_t*)(f->esp + WORD), (void*)*(uint32_t *)(f->esp+2*WORD),(unsigned)*(uint32_t*)(f->esp+3*WORD));
		break;
	case SYS_SEEK:                   /* Change position in a file. */
		break;
	case SYS_TELL:                   /* Report current position in a file. */
		break;
	case SYS_CLOSE:                  /* Close a file. */
		break;
		//####
	//case SYS_FIBBO:
	  //  break;
	//case SYS_SUM:
	  //  break;
		//$$$$
	default:
		printf("userprog/Syscall.c/Function System_Handler Error breaks out \n");
	}
}

void halt(void) {
	printf("userprog/syscall.c/halt start\n");
	shutdown_power_off();
}

void exit(int status) {
	printf("userprog/syscall.c/exit start\n");
	struct thread* cur = thread_current();
	printf("%s : exit(%d)", cur->name, status);
	printf("kk");
	cur->status = status;
	thread_exit();

}

pid_t exec(const char* cmd_lines) {
	printf("userprog/syscall.c/exec start\n");

	tid_t result = process_execute(cmd_lines);
	return (pid_t)(result - 1);
}

int wait(pid_t pid) {
	printf("userprog/syscall.c/wait start\n");
	return process_wait((tid_t)pid);
}

bool create(const char* file, unsigned initial_size);
bool remove(const char* file);
int open(const char* file);
int filesize(int fd);
int read(int fd, void* buffer, unsigned size); 

int write(int fd, const void* buffer, unsigned size) {
	// printf("write!!haha\n");
	 // fd는 파일 디스크립터 -> 파일이 오픈되고 나면 파일 디스크립터라는 ㅇ니덱스 번호가 반환된다.
	if (fd != STDOUT) {
		return -1;
	}

	// size
	putbuf(buffer, size);
	//  printf("\n ntest");
	return size;
}
void seek(int fd, unsigned position);
unsigned tell(int fd);
void close(int fd);
void check_address(void* addr) 
{
	if (!is_user_vaddr(vaddr)) 
	{
		exit(-1);
	}
}
//$$$$
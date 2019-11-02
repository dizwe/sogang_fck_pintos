#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "lib/user/syscall.h"
static void syscall_handler(struct intr_frame*);
void halt(void);
void exit(int status);
// lib/user/syscall.h��pid_t ����Ǿ�����
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
static void
syscall_handler(struct intr_frame* f UNUSED)
{
	uint32_t* args = f->esp;
//	printf("--- arg num : %d", args[0]);
	//  printf ("system call!\n");
	//  printf ("num : %d\n",*(uint32_t*)(f->esp));
	switch (*(uint32_t*)(f->esp)) {
	case SYS_HALT:                   /* Halt the operating system. */
		halt();
		break;
	case SYS_EXIT:					/* Terminate this process. */
		check_address(f->esp + WORD);
		//printf("exit num : %d",(int)args[1]);
		exit((int) * (uint32_t*)(f->esp + WORD));
		//exit((int)args[1]);
		break;
	case SYS_EXEC:                   /* Start another process. */
		check_address(f->esp + WORD);
		f->eax = exec((const char*) * (uint32_t*)(f->esp + WORD));
		break;
	case SYS_WAIT:                   /* Wait for a child process to die. */
		check_address(f->esp + WORD);
		f->eax = wait((pid_t) * (uint32_t*)(f->esp + WORD));
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
		check_address(f->esp+WORD); 
		f->eax = read((int)args[1], (void *)args[2], (unsigned)args[3]);
		break;
	case SYS_WRITE:                  /* Write to a file. */
		check_address(f->esp+WORD); 
  //	  printf("%d %d --data \n",args[1],args[3]);
		f->eax = write((int)args[1], (void*)args[2], (unsigned)args[3]);
		// write((int) * (uint32_t*)(f->esp + WORD), (void*)*(uint32_t *)(f->esp+2*WORD),(unsigned)*(uint32_t*)(f->esp+3*WORD));
		break;
	case SYS_SEEK:                   /* Change position in a file. */
		break;
	case SYS_TELL:                   /* Report current position in a file. */
		break;
	case SYS_CLOSE:                  /* Close a file. */
		break;
		//####
	case SYS_FIBBO:
		fibonacci((int)args[1]);
		break;
	case SYS_SUM:
		sum_of_four_int((int)args[1], (int)args[2], (int)args[3], (int)args[4]);
		break;
		//$$$$
	default:
		printf("userprog/Syscall.c/Function System_Handler Error breaks out \n");
	}
}

void halt(void) {
	//printf("userprog/syscall.c/halt start\n");
	shutdown_power_off();
}

void exit(int status) {
	//printf("userprog/syscall.c/exit start\n");
	struct thread* cur = thread_current();
    char real_file_name[128]; // 4kb?��?��?�� ��. 

    int idx=0;
    // �� ��?���� ��? 
    while((cur->name)[idx] != ' ' && (cur->name)[idx]!= '\0')
    {  real_file_name[idx] = (cur->name)[idx];
	   idx++;
    }
    // ���� ����
    real_file_name[idx]='\0';	
	
	printf("%s: exit(%d)\n", real_file_name, status);
	//printf("-----%d status ",status);
	// change ""child"" status
	cur->child_exit_status = status;
	thread_exit();

}

pid_t exec(const char* cmd_lines) {
	struct file *file = NULL;
	//printf("userprog/syscall.c/exec start\n");
//	printf("\n---start : %s",cmd_lines);
	// exec-missing checkes whether cmd_lines really exists
	file = filesys_open(cmd_lines);
	if(file ==NULL)
	{
		// because it can be child process I should return exit status , not exit directly
		return -1;
	}
	tid_t result = process_execute(cmd_lines);

	// child does not print out anything if do not wait
	process_wait(result);
	return (pid_t)(result - 1);
}

int wait(pid_t pid) {
	//printf("userprog/syscall.c/wait start\n");
	return process_wait((tid_t)pid);
}

bool create(const char* file, unsigned initial_size);
bool remove(const char* file);
int open(const char* file);
int filesize(int fd);
int read(int fd, void* buffer, unsigned size){
	int i;
	if(fd != STDIN){
		return -1;
	}
	for(i=0;i<size;i++){
		if(input_getc()=='\0'){
			break;
		}
	}

	return i;
}
int write(int fd, const void* buffer, unsigned size) {
	// printf("write!!haha\n");
	 // fd�� ���� ��ũ���� -> ������ ���µǰ� ���� ���� ��ũ���Ͷ�� ���ϵ��� ��ȣ�� ��ȯ�ȴ�.
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
	if (!is_user_vaddr(addr)) 
	{
		exit(-1);
	}
}
//$$$$


int fibonacci(int n) {
	int a, b, ff, i;
	if (n == 0) return 0;
	if (n == 1) return 1;

	for (i = 2; i <= n; i++) {
		ff = a + b;
		a = b;
		b = ff;
	}
	return ff;
}

int sum_of_four_int(int a, int b, int c, int d) {
	return a + b + c + d;
}
//$$$$


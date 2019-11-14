#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <stdlib.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "lib/user/syscall.h"
#include "filesys/off_t.h"
//#include "filesys/filesys.c"
//#include "filesys/file.c"
struct semaphore mutex, wrt;
int readcount = 0;

struct file{
	struct inode * inode;
	off_t pos;
	bool deny_write;
}file;
static void syscall_handler(struct intr_frame*);
void halt(void);
void exit(int status);
// lib/user/syscall.h¿¡pid_t ÀúÀåµÇ¾îÀÖÀ½
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
char * file_name_parser(const char * file);
void fd_check(int fd);

#define WORD sizeof(uint32_t)
#define STDIN 0
#define STDOUT 1
#define STDERR 2

void
syscall_init (void) 
{
  sema_init(&mutex, 1);
  sema_init(&wrt, 1);
  readcount = 0;
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
	//	printf("\n --- wait eax : %d\n",f->eax);
		break;
	case SYS_CREATE:                 /* Create a file. */
		check_address(f->esp + WORD);
		check_address(f->esp + WORD + WORD);
		f->eax = create((const char*)args[1], (unsigned)args[2] );
		break;
	case SYS_REMOVE:                 /* Delete a file. */
		check_address(f->esp+WORD);
		f->eax = remove((const char *)args[1]);
		break;
	case SYS_OPEN:                   /* Open a file. */
		check_address(f->esp + WORD);
		f->eax = open((const char *) args[1]);
		break;
	case SYS_FILESIZE:               /* Obtain a file's size. */
		check_address(f->esp + WORD);
		f->eax = filesize((int)args[1]);
		break;
	case SYS_READ:                   /* Read from a file. */
		check_address(f->esp+WORD);
		check_address(f->esp+WORD+WORD);
		check_address(f->esp+WORD+WORD+WORD); 
		f->eax = read((int)args[1], (void *)args[2], (unsigned)args[3]);
		break;
	case SYS_WRITE:                  /* Write to a file. */
		//printf("hihihihi");
		check_address(f->esp+WORD); 
		check_address(f->esp+WORD+WORD);
		check_address(f->esp+WORD+WORD+WORD);
  //	  printf("%d %d --data \n",args[1],args[3]);
		f->eax = write((int)args[1], (void*)args[2], (unsigned)args[3]);
		// write((int) * (uint32_t*)(f->esp + WORD), (void*)*(uint32_t *)(f->esp+2*WORD),(unsigned)*(uint32_t*)(f->esp+3*WORD));
		break;
	case SYS_SEEK:                   /* Change position in a file. */
		check_address(f->esp+WORD);
		check_address(f->esp + WORD + WORD);
		seek((int) args[1], (unsigned) args[2]);
		break;
	case SYS_TELL:                   /* Report current position in a file. */
		check_address(f->esp+WORD);
		f->eax = tell((int)args[1]);
		break;
	case SYS_CLOSE:                  /* Close a file. */
		check_address(f->esp + WORD);
		close((int)args[1]);
		break;
		//####
	case SYS_FIBBO:
		check_address(f->esp+WORD);
		f->eax = fibo((int)args[1]);
		break;
	case SYS_SUM:
		check_address(f->esp + WORD);
		check_address(f->esp + WORD + WORD);
		check_address(f->esp + WORD + WORD + WORD);
		check_address(f->esp + WORD + WORD + WORD + WORD);
		f->eax = sum__((int)args[1], (int)args[2], (int)args[3], (int)args[4]);
		break;
		//$$$$
	default:
		printf("userprog/Syscall.c/Function System_Handler Error breaks out \n");
	}
}

void halt(void) {
	shutdown_power_off();
}

void exit(int status) {
    struct thread* cur = thread_current();
    char real_file_name[128];

    int idx=0, i;
    while((cur->name)[idx] != ' ' && (cur->name)[idx]!= '\0')
    {  real_file_name[idx] = (cur->name)[idx];
	   idx++;
    }
    real_file_name[idx]='\0';	
	
	printf("%s: exit(%d)\n", real_file_name, status);
	cur->child_exit_status = status;
	
	struct thread * current_thread = thread_current();
	for (i = 3; i < 128; i++){
		if(current_thread->file_descriptor[i] != NULL){
			close(i);
		}
	}
	thread_exit();

}

pid_t exec(const char* cmd_lines) {
	struct file *file = NULL;
	int idx = 0;
	char real_file_name[128];
	while(cmd_lines[idx] != ' ' && cmd_lines[idx]!='\0'){
		real_file_name[idx] = cmd_lines[idx]; 
		idx++;
	}
	real_file_name[idx]='\0';
	file = filesys_open(real_file_name);
	
	if(file ==NULL)
	{
		return -1;
	}
	tid_t result = process_execute(cmd_lines);
	return (pid_t)result;
}

int wait(pid_t pid) {
	return process_wait((tid_t)pid);
}

bool create(const char* file, unsigned initial_size){
	if (file == NULL) exit(-1);
	check_address(file);

	return filesys_create(file, initial_size);
}

bool remove(const char* file){
	if(file == NULL) exit ( -1);
	check_address(file);
//	int i;
//	char *file_name;
	
//	for(i = 0; file[i] != ' ' && file[i] != '\0'; i++);
//	file_name = (char *)malloc(sizeof(char) * (i + 1));
//	strlcpy(file_name, file, i);
//	file_name[i] = '\0';
	
//	free(file_name);
	return filesys_remove(file);
}

int open(const char* file){
	if(file == NULL) exit(-1);
	check_address(file);
	int i;
	char * file_name;
	file_name = file_name_parser(file);
	struct file * opening_file = filesys_open(file);
	if(opening_file == NULL) return -1;
	else if (opening_file != NULL){
		for(i = 3; i < 128; i++){
			if(thread_current()->file_descriptor[i] == NULL){
				thread_current()->file_descriptor[i] = opening_file;
				// ¿¿ ¿¿¿¿ thread¿ ¿¿¿ ¿¿¿¿¿ ¿¿¿ ¿¿¿ 
				// ¿¿ ¿¿¿ ¿¿¿ ¿¿.
				if(!strcmp(file, thread_current()->name)){
					file_deny_write(opening_file);
				}
				
				return i;
			}
		}	
	}
	else {
		printf("Error breaks out at open file\n");
		exit(-1);
	}

}

int filesize(int fd){
	struct thread * current_thread = thread_current();
	int i;
//	if(current_thread->file_descriptor[fd] == NULL) exit(-1);
	fd_check(fd);
	return (int)file_length(thread_current()->file_descriptor[fd]);
}

int read(int fd, void* buffer, unsigned size){

	int i=0, ret = -1;
	// READ & WRITED PROBLEM
	sema_down(&mutex);
	readcount++;
	if(readcount == 1)
		sema_down(&wrt);
	sema_up(&mutex);
	// MUTEX LOCK

	check_address(buffer);
//	struct thread * current_thread = thread_current();
//	if(current_trhead -> file_descriptor[fd] == NULL) exit(-);
//	fd_check(fd);
	if(fd == 0){
		for(i = 0; i < size; i++){
			if(input_getc() == '\0'){
				break;
			}
		}	
	}
	
	else if(fd > 2)	{
	//	fd_check(fd);
		struct thread * cur_thread = thread_current();
	//	file_deny_write(cur_thread->file_descriptor[fd]);
	//	if(cur_thread->file_descriptor[fd]->deny_write){
	//		file_deny_write(cur_thread->file_descriptor[fd]);
		//	ret = file_read(thread_current()->file_descriptor[fd], buffer, size);
	//	}
		ret = file_read(cur_thread->file_descriptor[fd],buffer,size );
//		file_allow_write(cur_thread->file_descriptor[fd]);
//		if(cur_thread->file_descriptor[fd]->deny_write
		
		i = ret;
	}
	else{
		i = -1;
	//	exit(-1);
	}
	sema_down(&mutex);
	readcount--;
	if(readcount == 0)
		sema_up(&wrt);
	sema_up(&mutex);

	return i;
}

int write(int fd, const void* buffer, unsigned size) {
	sema_down(&wrt);
	check_address(buffer);
	// printf("write!!haha\n");
//	struct thread * current_thread = thread_current();
//	if(current_thread -> file_descriptor[fd] == NULL) exit(-1);
//	fd_check(fd);
	int ret;
	if (fd == 1) {	
		putbuf(buffer, size);
		ret = size;
	}
	else if ( fd > 2){
		fd_check(fd);
		struct thread * cur_thread = thread_current();
		struct file * cur_file = cur_thread->file_descriptor[fd];
		if(cur_file->deny_write){
			file_deny_write(cur_file);
		}
	//	file_deny_write(cur_thread->file_descriptor[fd]);
		ret = file_write(thread_current()->file_descriptor[fd], buffer, size);
	//	file_allow_write(cur_thread->file_descriptor[fd]);
		
	}
	else{
		ret = -1;
	}
	sema_up(&wrt);

	return ret;
}

void seek(int fd, unsigned position){
//	struct thread * current_thread = thread_current();
//	if(current_thread->file_descriptor[fd] == NULL) exit(-1);

	fd_check(fd);
	return file_seek(thread_current()->file_descriptor[fd], position);
}

unsigned tell(int fd){
	fd_check(fd);
	return (unsigned)file_tell(thread_current()->file_descriptor[fd]);
}

void close(int fd){
//	fd_check(fd);
//	int i;
	struct thread * cur_thread = thread_current();
	struct file * fp = cur_thread->file_descriptor[fd];
//	file_allow_write(fp);
// 	file_close(thread_current()->file_descriptor[fd]);
	
	if(cur_thread -> file_descriptor[fd] == NULL) exit(-1);
	cur_thread->file_descriptor[fd] = NULL;
	file_close(fp);	
}

void check_address(void* addr) 
{
	if (!is_user_vaddr(addr)) 
	{
		exit(-1);
	}
}
//$$$$


int fibo(int n) {
	int a, b, ff, i;
	a= 0 ; b = 1;
	if (n == 0) return 0;
	if (n == 1) return 1;
	for (i = 2; i <= n; i++) {
		ff = a + b;
		a = b;
		b = ff;
	}
	return ff;
}

int sum__(int a, int b, int c, int d) {
	return a + b + c + d;
}
//$$$$

// Input : ¿¿ ¿¿ ¿¿
// OutPut : input ¿¿ ¿¿ ¿¿
char * file_name_parser(const char * file){
	int i;
	char *file_name;
	for(i = 0; file[i] != ' ' && file[i] != '\0'; i++);
	file_name = (char *)malloc(sizeof(char) * (i + 1));
	strlcpy(file_name, file, i);
	file_name[i] = '\0';
	return file_name;
}

void fd_check(int fd){
	if(thread_current()->file_descriptor[fd] == NULL) exit(-1);
	return;
}


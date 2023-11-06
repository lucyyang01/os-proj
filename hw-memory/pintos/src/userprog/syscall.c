#include "userprog/syscall.h"
#include <stdio.h>
#include <string.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "threads/palloc.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"

static void syscall_handler(struct intr_frame*);

void syscall_init(void) { intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall"); }

void syscall_exit(int status) {
  printf("%s: exit(%d)\n", thread_current()->name, status);
  thread_exit();
}

/*
 * This does not check that the buffer consists of only mapped pages; it merely
 * checks the buffer exists entirely below PHYS_BASE.
 */
static void validate_buffer_in_user_region(const void* buffer, size_t length) {
  uintptr_t delta = PHYS_BASE - buffer;
  if (!is_user_vaddr(buffer) || length > delta)
    syscall_exit(-1);
}

/*
 * This does not check that the string consists of only mapped pages; it merely
 * checks the string exists entirely below PHYS_BASE.
 */
static void validate_string_in_user_region(const char* string) {
  uintptr_t delta = PHYS_BASE - (const void*)string;
  if (!is_user_vaddr(string) || strnlen(string, delta) == delta)
    syscall_exit(-1);
}

static int syscall_open(const char* filename) {
  struct thread* t = thread_current();
  if (t->open_file != NULL)
    return -1;

  t->open_file = filesys_open(filename);
  if (t->open_file == NULL)
    return -1;

  return 2;
}

static int syscall_write(int fd, void* buffer, unsigned size) {
  struct thread* t = thread_current();
  if (fd == STDOUT_FILENO) {
    putbuf(buffer, size);
    return size;
  } else if (fd != 2 || t->open_file == NULL)
    return -1;

  return (int)file_write(t->open_file, buffer, size);
}

static int syscall_read(int fd, void* buffer, unsigned size) {
  struct thread* t = thread_current();
  if (fd != 2 || t->open_file == NULL)
    return -1;

  return (int)file_read(t->open_file, buffer, size);
}

static void syscall_close(int fd) {
  struct thread* t = thread_current();
  if (fd == 2 && t->open_file != NULL) {
    file_close(t->open_file);
    t->open_file = NULL;
  }
}

static uint8_t* syscall_sbrk(intptr_t increment) {
  struct thread* t = thread_current();
  if (increment == 0)
    return t->segbreak;
  if (increment > 0) {
    if (pg_round_up(t->segbreak + increment) != pg_round_up(t->segbreak)) {
      uint32_t rounded = pg_round_up(t->segbreak + increment) - pg_round_up(t->segbreak);
      uint32_t num_pages = rounded / PGSIZE;
      uint8_t* kpages = palloc_get_multiple(PAL_USER | PAL_ZERO, num_pages);
      if (kpages == NULL)
        return (void*) -1;
      for(int i = 0; i < num_pages; i++) {
        install_page(pg_round_up(t->segbreak) + (i * PGSIZE), kpages + (i * PGSIZE), true);
        // if (!success) {
        //   for (int j = 0; j < i + 1; j++) {
        //      uint8_t* actual_page = pagedir_get_page(t->pagedir, pg_round_down(t->segbreak) - (j * PGSIZE));
        //     pagedir_clear_page(t->pagedir, pg_round_down(t->segbreak) - (j * PGSIZE));
        //     palloc_free_page(actual_page);
        //   }
        // }
      }
    }
  } else {
    if (pg_round_up(t->segbreak + increment) != pg_round_up(t->segbreak)) {
      uint32_t num_pages = ((pg_round_up(t->segbreak + increment) - pg_round_up(t->segbreak)) * -1) / PGSIZE;
      for(int i = 0; i < num_pages; i++) {
        uint8_t* actual_page = pagedir_get_page(t->pagedir, pg_round_down(t->segbreak) - (i * PGSIZE));
        pagedir_clear_page(t->pagedir, pg_round_down(t->segbreak) - (i * PGSIZE));
        palloc_free_page(actual_page);
      }
    }
  }
  uint8_t* old = t->segbreak;
  t->segbreak += increment;
  return old;
}


static void syscall_handler(struct intr_frame* f) {
  uint32_t* args = (uint32_t*)f->esp;
  struct thread* t = thread_current();
  t->in_syscall = true;

  validate_buffer_in_user_region(args, sizeof(uint32_t));
  switch (args[0]) {
    case SYS_EXIT:
      validate_buffer_in_user_region(&args[1], sizeof(uint32_t));
      syscall_exit((int)args[1]);
      break;

    case SYS_OPEN:
      validate_buffer_in_user_region(&args[1], sizeof(uint32_t));
      validate_string_in_user_region((char*)args[1]);
      f->eax = (uint32_t)syscall_open((char*)args[1]);
      break;

    case SYS_WRITE:
      validate_buffer_in_user_region(&args[1], 3 * sizeof(uint32_t));
      validate_buffer_in_user_region((void*)args[2], (unsigned)args[3]);
      f->eax = (uint32_t)syscall_write((int)args[1], (void*)args[2], (unsigned)args[3]);
      break;

    case SYS_READ:
      validate_buffer_in_user_region(&args[1], 3 * sizeof(uint32_t));
      validate_buffer_in_user_region((void*)args[2], (unsigned)args[3]);
      f->eax = (uint32_t)syscall_read((int)args[1], (void*)args[2], (unsigned)args[3]);
      break;

    case SYS_CLOSE:
      validate_buffer_in_user_region(&args[1], sizeof(uint32_t));
      syscall_close((int)args[1]);
      break;

    case SYS_SBRK:
      validate_buffer_in_user_region(&args[1], sizeof(uint32_t));
      f->eax = (uint32_t) syscall_sbrk((intptr_t) args[1]);
      break;
    
    default:
      printf("Unimplemented system call: %d\n", (int)args[0]);
      break;
  }

  t->in_syscall = false;
}

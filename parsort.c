#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int compare_i64(const void *left_, const void *right_) {
  int64_t left = *(int64_t *)left_;
  int64_t right = *(int64_t *)right_;
  if (left < right) return -1;
  if (left > right) return 1;
  return 0;
}

void seq_sort(int64_t *arr, size_t begin, size_t end) {
  size_t num_elements = end - begin;
  qsort(arr + begin, num_elements, sizeof(int64_t), compare_i64);
}

// Merge the elements in the sorted ranges [begin, mid) and [mid, end),
// copying the result into temparr.
void merge(int64_t *arr, size_t begin, size_t mid, size_t end, int64_t *temparr) {
  int64_t *endl = arr + mid;
  int64_t *endr = arr + end;
  int64_t *left = arr + begin, *right = arr + mid, *dst = temparr;

  for (;;) {
    int at_end_l = left >= endl;
    int at_end_r = right >= endr;

    if (at_end_l && at_end_r) break;

    if (at_end_l)
      *dst++ = *right++;
    else if (at_end_r)
      *dst++ = *left++;
    else {
      int cmp = compare_i64(left, right);
      if (cmp <= 0)
        *dst++ = *left++;
      else
        *dst++ = *right++;
    }
  }
}

void fatal(const char *msg) __attribute__ ((noreturn));

void fatal(const char *msg) {
  fprintf(stderr, "Error: %s\n", msg);
  exit(1);
}

void merge_sort(int64_t *arr, size_t begin, size_t end, size_t threshold) {
  assert(end >= begin);
  size_t size = end - begin;

  if (size <= threshold) {
    seq_sort(arr, begin, end);
    return;
  }

  // recursively sort halves in parallel

  size_t mid = begin + size/2;

  // parallelize the recursive sorting
  pid_t leftFork = fork();
  if (leftFork == -1) {
    fatal("left fork failed to start new process");
  } else if (leftFork == 0) {
    // in child process
    merge_sort(arr, begin, mid, threshold);
    exit(0); // kill child process
  }
  // continue if in parent process...

  pid_t rightFork = fork();
  if (rightFork == -1) {
    fatal("right fork failed to start new process");
  } else if (rightFork == 0) {
    // in child process
    merge_sort(arr, mid, end, threshold);
    exit(0); // kill child process
  }
  // continue if in parent process...

  // pause program execution until a child process has completed
  int leftForkwStatus;
  int rightForkwStatus;
  pid_t actualLeft = waitpid(leftFork, &leftForkwStatus, 0);
  pid_t actualRight = waitpid(rightFork, &rightForkwStatus, 0);
  if (actualLeft == -1) { 
    fatal("left waitpid failed");
  }
  if (actualRight == -1) {
    fatal("right waitpid failed");
  }
  if (!WIFEXITED(leftForkwStatus)) {
    fatal("left child failed");
  }
  if (WEXITSTATUS(leftForkwStatus) != 0) {
    fatal("left child exit failed");
  }
  if (!WIFEXITED(rightForkwStatus)) {
    fatal("right child failed");
  }
  if (WEXITSTATUS(rightForkwStatus) != 0) {
    fatal("right child exit failed");
  }

  // allocate temp array now, so we can avoid unnecessary work
  // if the malloc fails
  int64_t *temp_arr = (int64_t *) malloc(size * sizeof(int64_t));
  if (temp_arr == NULL) {
    fatal("malloc() failed");
  }

  // child processes completed successfully, so in theory
  // we should be able to merge their results
  merge(arr, begin, mid, end, temp_arr);

  // copy data back to main array
  for (size_t i = 0; i < size; i++)
    arr[begin + i] = temp_arr[i];

  // now we can free the temp array
  free(temp_arr);

  // success!
}

int main(int argc, char **argv) {
  // check for correct number of command line arguments
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <filename> <sequential threshold>\n", argv[0]);
    return 1;
  }

  // process command line arguments
  const char *filename = argv[1];
  char *end;
  size_t threshold = (size_t) strtoul(argv[2], &end, 10);
  if (end != argv[2] + strlen(argv[2])) {
    fatal("threshold value is invalid");
  }

  // open the file
  int fd = open(filename, O_RDWR);
  if (fd < 0) {
    fatal("file could not be opened");
  }

  // use fstat to determine the size of the file
  struct stat statbuf;
  int rc = fstat(fd, &statbuf);
  if (rc != 0) {
    close(fd);
    fatal("unable to determine file size");
  }
  size_t file_size_in_bytes = statbuf.st_size;

  // map the file into memory using mmap
  int64_t *data = mmap(NULL, file_size_in_bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (data == MAP_FAILED) {
    fatal("unable to map file to memory");
  }
  
  // close the file
  int closeResult = close(fd);
  if (closeResult == -1) {
    fatal("unable to close fd");
  }
  
  // sort the data!
  size_t length = file_size_in_bytes / sizeof(int64_t);
  merge_sort(data, 0, length, threshold);

  // unmap the data
  int munmapResult = munmap(data, file_size_in_bytes);
  if (munmapResult == -1) {
    fatal("unable to unmap file from memory");
  }

  // exit with a 0 exit code if sort was successful
  return 0;
}

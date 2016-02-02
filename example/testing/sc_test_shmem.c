/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2015 The University of Texas System

  The SC Library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  The SC Library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with the SC Library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
*/

#include <sc_refcount.h>
#include <sc_shmem.h>

#define DATA_SIZE 10

#include <spi/include/kernel/memory.h>

void print_memusage()
{
  uint64_t shared, persist, heapavail, stackavail, stack, heap, guard, mmap;

  Kernel_GetMemorySize(KERNEL_MEMSIZE_GUARD, &guard);
  Kernel_GetMemorySize(KERNEL_MEMSIZE_SHARED, &shared);
  Kernel_GetMemorySize(KERNEL_MEMSIZE_PERSIST, &persist);
  Kernel_GetMemorySize(KERNEL_MEMSIZE_HEAPAVAIL, &heapavail);
  Kernel_GetMemorySize(KERNEL_MEMSIZE_STACKAVAIL, &stackavail);
  Kernel_GetMemorySize(KERNEL_MEMSIZE_STACK, &stack);
  Kernel_GetMemorySize(KERNEL_MEMSIZE_HEAP, &heap);
  Kernel_GetMemorySize(KERNEL_MEMSIZE_MMAP, &mmap);
#if 0
  printf("Allocated heap: %.2f MB, avail. heap: %.2f MB\n", double(heap)/(1024*1024), double(heapavail)/(1024*1024));
  printf("Allocated stack: %.2f MB, avail. stack: %.2f MB\n", double(stack)/(1024*1024), double(stackavail)/(1024*1024));
  printf("Memory: shared: %.2f MB, persist: %.2f MB, guard: %.2f MB, mmap: %.2f MB\n", double(shared)/(1024*1024), double(persist)/(1024*1024), double(guard)/(1024*1024), double(mmap)/(1024*1024));
#else
  printf("MEMSIZE heap: %.2f/%.2f stack: %.2f/%.2f mmap: %.2f MB\n", (double)heap/(1024*1024), (double)heapavail/(1024*1024), (double)stack/(1024*1024), (double)stackavail/(1024*1024), (double)mmap/(1024*1024));
  printf("MEMSIZE shared: %.2f persist: %.2f guard: %.2f MB\n", (double)shared/(1024*1024), (double)persist/(1024*1024), (double)guard/(1024*1024));
#endif
}


/* This struct stores data which we use to test shared
 * memory arrays.
 */
typedef struct
{
  int                 rank;     /*< This entry stores the rank of the creating process */
  double              data[DATA_SIZE];  /*< This field can store arbitrary data */
} data_t;

#if 0
/* For each process print the integer entry of
 * each element in an array of type data_t.
 */
void
test_shmem_print_int (data_t * array, sc_MPI_Comm comm)
{
  int                 i, p;
  MPI_Aint            address;
  int                 mpirank, mpisize, mpiret;
  char                outstring[BUFSIZ];

  mpiret = sc_MPI_Comm_size (comm, &mpisize);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_rank (comm, &mpirank);
  SC_CHECK_MPI (mpiret);

  mpiret = MPI_Get_address ((void *) array, &address);
  SC_CHECK_MPI (mpiret);
  outstring[0] = '\0';
  snprintf (outstring + strlen (outstring), BUFSIZ - strlen (outstring),
            "Array at %li:\t", (long) address);
  for (i = 0; i < mpisize; i++)
    /* loop over array entries */
  {
    snprintf (outstring + strlen (outstring), BUFSIZ - strlen (outstring),
              "%i ", array[i].rank);
  }

  for (p = 0; p < mpisize; p++)
    /* loop over procs */
  {
    if (mpirank == p) {
      printf ("[H %i] %s\n", mpirank, outstring);
      outstring[0] = '\0';
      fflush (stdout);
    }
    sc_MPI_Barrier (comm);
  }
}
#endif

/* Check whether a given data item has entries
 * data.rank = i
 * data.data = {0,...,DATA_SIZE-1}
 */
int
test_shmem_correct_data (data_t * data, int i)
{
  int                 j;
  if (data->rank != i) {
    return 0;
  }
  for (j = 0; j < DATA_SIZE; j++) {
    if (data->data[j] != (double) j) {
      return 0;
    }
  }
  return 1;
}

/* Fill the array of one data item with
 * the numbers 0,...,DATA_SIZE -1.
 */
void
test_shmem_fill_data (data_t * data)
{
  int                 i;

  for (i = 0; i < DATA_SIZE; i++) {
    data->data[i] = (double) i;
  }
}

/* allocate a shared memory array and fill the date fields */
data_t *
test_shmem_create_data_array (sc_shmem_type_t type, int mpirank, int mpisize)
{
  data_t              data;
  data_t             *data_array;
  int                 i;

  data.rank = mpirank;
  test_shmem_fill_data (&data);

  sc_shmem_set_type (sc_MPI_COMM_WORLD, type);

print_memusage();
  data_array = SC_SHMEM_ALLOC (data_t, mpisize, sc_MPI_COMM_WORLD);
  SC_CHECK_ABORT (data_array != NULL, "Allocation failed");
print_memusage();

  sc_shmem_allgather (&data, sizeof (data_t), sc_MPI_BYTE, data_array,
                      sizeof (data_t), sc_MPI_BYTE, sc_MPI_COMM_WORLD);
  /* check whether creation worked */
  for (i = 0; i < mpisize; i++) {
    SC_CHECK_ABORTF (test_shmem_correct_data (&data_array[i], i),
                     "Error in shmem_allgather. Array entry %i is not correct.",
                     i);
  }
  return data_array;
}

/* For a given shmem type, allocate a shared array
 * and fill it with data via a call to shmem_allgather.
 * We check wether all data was gathered correctly and
 * free the array.
 */
void
test_shmem_allgather (sc_shmem_type_t type)
{
  data_t             *data_array;
  int                 mpirank, mpisize, mpiret;
  int                 i;

  SC_GLOBAL_ESSENTIALF ("Testing allgather with type %s.\n",
                        sc_shmem_type_to_string[type]);
  mpiret = sc_MPI_Comm_size (sc_MPI_COMM_WORLD, &mpisize);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_rank (sc_MPI_COMM_WORLD, &mpirank);
  SC_CHECK_MPI (mpiret);

  data_array = test_shmem_create_data_array (type, mpirank, mpisize);

  for (i = 0; i < mpisize; i++) {
    SC_CHECK_ABORTF (test_shmem_correct_data (&data_array[i], i),
                     "Error in shmem_allgather. Array entry %i is not correct.",
                     i);
  }
  SC_SHMEM_FREE (data_array, sc_MPI_COMM_WORLD);

  SC_GLOBAL_ESSENTIALF ("Testing type %s succesful.\n",
                        sc_shmem_type_to_string[type]);
}

/* create a shmem array, copy it and check whether the
 * copy is the same as the original
 */
void
test_shmem_copy (sc_shmem_type_t type)
{
  data_t             *data_array, *copy_array;
  int                 mpirank, mpisize, mpiret;
  int                 i;

  SC_GLOBAL_ESSENTIALF ("Testing copy with type %s.\n",
                        sc_shmem_type_to_string[type]);
  mpiret = sc_MPI_Comm_size (sc_MPI_COMM_WORLD, &mpisize);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_rank (sc_MPI_COMM_WORLD, &mpirank);
  SC_CHECK_MPI (mpiret);

  data_array = test_shmem_create_data_array (type, mpirank, mpisize);
  copy_array = SC_SHMEM_ALLOC (data_t, mpisize, sc_MPI_COMM_WORLD);

  sc_shmem_memcpy ((void *) copy_array, (void *) data_array,
                   mpisize * sizeof (data_t), sc_MPI_COMM_WORLD);
  /* Check whether the copy worked */
  for (i = 0; i < mpisize; i++) {
    SC_CHECK_ABORTF (!memcmp
                     (&data_array[i], &copy_array[i], sizeof (data_t)),
                     "Error in shmem_copy. Array entries at %i do not match",
                     i);
  }

  SC_SHMEM_FREE (data_array, sc_MPI_COMM_WORLD);
  SC_SHMEM_FREE (copy_array, sc_MPI_COMM_WORLD);

  SC_GLOBAL_ESSENTIALF ("Testing type %s succesful.\n",
                        sc_shmem_type_to_string[type]);
}

void
test_shmem_write (sc_shmem_type_t type)
{
  data_t             *data_array;
  int                 mpirank, mpisize, mpiret;
  int                 i;

  SC_GLOBAL_ESSENTIALF ("Testing shmem_write with type %s.\n",
                        sc_shmem_type_to_string[type]);
  mpiret = sc_MPI_Comm_size (sc_MPI_COMM_WORLD, &mpisize);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_rank (sc_MPI_COMM_WORLD, &mpirank);
  SC_CHECK_MPI (mpiret);

  data_array = SC_SHMEM_ALLOC (data_t, mpisize, sc_MPI_COMM_WORLD);
  /* The process that first gets write access to the array writes the data */
  if (sc_shmem_write_start (data_array, sc_MPI_COMM_WORLD)) {
    for (i = 0; i < mpisize; i++) {
      data_array[i].rank = i;
      test_shmem_fill_data (&data_array[i]);
    }
  }

  sc_shmem_write_end (data_array, sc_MPI_COMM_WORLD);
  mpiret = sc_MPI_Barrier (sc_MPI_COMM_WORLD);
  SC_CHECK_MPI (mpiret);

  /* All processes check whether writing worked */
  for (i = 0; i < mpisize; i++) {
    SC_CHECK_ABORTF (test_shmem_correct_data (&data_array[i], i),
                     "Error in shmem_copy. Array entries at %i do not match",
                     i);
  }

  SC_SHMEM_FREE (data_array, sc_MPI_COMM_WORLD);
  SC_GLOBAL_ESSENTIALF ("Testing type %s succesful.\n",
                        sc_shmem_type_to_string[type]);
}

void
test_shmem_prefix (sc_shmem_type_t type)
{
  int                *data_array;
  int                 mpirank, mpisize, mpiret;
  int                 i;

  SC_GLOBAL_ESSENTIALF ("Testing prefix with type %s.\n",
                        sc_shmem_type_to_string[type]);
  mpiret = sc_MPI_Comm_size (sc_MPI_COMM_WORLD, &mpisize);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_rank (sc_MPI_COMM_WORLD, &mpirank);
  SC_CHECK_MPI (mpiret);

  data_array = SC_SHMEM_ALLOC (int, mpisize + 1, sc_MPI_COMM_WORLD);

  sc_shmem_prefix (&mpirank, data_array, 1, sc_MPI_INT, sc_MPI_SUM,
                   sc_MPI_COMM_WORLD);

  for (i = 0; i <= mpisize; i++) {
    SC_CHECK_ABORTF (data_array[i] == i * (i - 1) / 2,
                     "Error in shmem prefix."
                     "Array entry at %i is not correct.\n", i);
  }

  SC_SHMEM_FREE (data_array, sc_MPI_COMM_WORLD);
  SC_GLOBAL_ESSENTIALF ("Testing type %s succesful.\n",
                        sc_shmem_type_to_string[type]);
}

void
test_shmem_test1 ()
{
  int                 type;

  SC_GLOBAL_ESSENTIAL ("Testing sc_shmem_allgather.\n");
  sc_log_indent_push ();
  for (type = (int) SC_SHMEM_BASIC; type < (int) SC_SHMEM_NUM_TYPES; type++) {
    test_shmem_allgather ((sc_shmem_type_t) type);
  }
  sc_log_indent_pop ();
  SC_GLOBAL_ESSENTIAL ("Testing sc_shmem_copy.\n");
  sc_log_indent_push ();
  for (type = (int) SC_SHMEM_BASIC; type < (int) SC_SHMEM_NUM_TYPES; type++) {
    test_shmem_copy ((sc_shmem_type_t) type);
  }
  sc_log_indent_pop ();
  SC_GLOBAL_ESSENTIAL ("Testing sc_shmem_write.\n");
  sc_log_indent_push ();
  for (type = (int) SC_SHMEM_BASIC; type < (int) SC_SHMEM_NUM_TYPES; type++) {
    test_shmem_write ((sc_shmem_type_t) type);
  }
  sc_log_indent_pop ();
  SC_GLOBAL_ESSENTIAL ("Testing sc_shmem_prefix.\n");
  sc_log_indent_push ();
  for (type = (int) SC_SHMEM_BASIC; type < (int) SC_SHMEM_NUM_TYPES; type++) {
    test_shmem_prefix ((sc_shmem_type_t) type);
  }
  sc_log_indent_pop ();
}

int
main (int argc, char *argv[])
{
  int                 mpiret;

  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);

  sc_init (sc_MPI_COMM_WORLD, 1, 1, NULL, SC_LP_ESSENTIAL);

  test_shmem_test1 ();

  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);
  return 0;
}

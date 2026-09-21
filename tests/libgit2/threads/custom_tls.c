#include "clar_libgit2.h"

#include "thread_helpers.h"
#include "alloc.h"
#include "common.h"
#include "git2/sys/custom_tls.h"

#ifdef GIT_THREADS
static int *test[2] = { NULL, NULL };
static int num_threads_spawned = 0;
#endif

#if defined(GIT_THREADS) && defined(GIT_WIN32)
static DWORD _fls_index;

static int init_thread_local_storage(void)
{
  if ((_fls_index = FlsAlloc(NULL)) == FLS_OUT_OF_INDEXES)
    return -1;

  return 0;
}

static void cleanup_thread_local_storage(void)
{
  FlsFree(_fls_index);
}

static void *init_local_storage(void) {
  test[num_threads_spawned] = git__calloc(1, sizeof(int));
  return test[num_threads_spawned++];
}

static void init_tls(void *payload) {
  int *i = payload;
  (*i)++;
  FlsSetValue(_fls_index, i);
}

static void teardown_tls(void) {
  int *i = FlsGetValue(_fls_index);
  (*i)++;
}

#elif defined(GIT_THREADS) && defined(_POSIX_THREADS)
static pthread_key_t _tls_key;

static int init_thread_local_storage(void)
{
  return pthread_key_create(&_tls_key, NULL);
}

static void cleanup_thread_local_storage(void)
{
  pthread_key_delete(_tls_key);
}

static void *init_local_storage(void) {
  test[num_threads_spawned] = git__calloc(1, sizeof(int));
  return test[num_threads_spawned++];
}

static void init_tls(void *payload) {
  int *i = payload;
  (*i)++;
  pthread_setspecific(_tls_key, i);
}

static void teardown_tls(void) {
  int *i = pthread_getspecific(_tls_key);
  (*i)++;
}

#endif

void test_threads_custom_tls__initialize(void)
{
#ifdef GIT_THREADS
  cl_git_pass(init_thread_local_storage());
  cl_git_pass(git_custom_tls_set_callbacks(init_local_storage, init_tls, teardown_tls));
	test[0] = NULL;
  test[1] = NULL;
  num_threads_spawned = 0;
#endif
}

void test_threads_custom_tls__cleanup(void)
{
#ifdef GIT_THREADS
  cleanup_thread_local_storage();
  git_custom_tls_set_callbacks(NULL, NULL, NULL);

  git__free(test[0]);
  test[0] = NULL;

  git__free(test[1]);
  test[1] = NULL;
#endif
}

#ifdef GIT_THREADS
static void *return_normally(void *param)
{
  return param;
}
#endif

void test_threads_custom_tls__multiple_clean_exit(void)
{
#ifndef GIT_THREADS
  clar__skip();
#else
  git_thread thread1, thread2;
  void *result;

  cl_git_pass(git_thread_create(&thread1, return_normally, (void *)424242));
  cl_git_pass(git_thread_create(&thread2, return_normally, (void *)232323));

  cl_git_pass(git_thread_join(&thread1, &result));
	cl_assert_equal_sz(424242, (size_t)result);
  cl_git_pass(git_thread_join(&thread2, &result));
	cl_assert_equal_sz(232323, (size_t)result);

  cl_assert_equal_i(2, *(test[0]));
  cl_assert_equal_i(2, *(test[1]));
#endif
}

#ifdef GIT_THREADS
static void *return_early(void *param)
{
  git_thread_exit(param);
  assert(false);
  return param;
}
#endif

void test_threads_custom_tls__multiple_threads_use_exit(void)
{
#ifndef GIT_THREADS
  clar__skip();
#else
  git_thread thread1, thread2;
  void *result;

  cl_git_pass(git_thread_create(&thread1, return_early, (void *)424242));
  cl_git_pass(git_thread_create(&thread2, return_early, (void *)232323));

  cl_git_pass(git_thread_join(&thread1, &result));
	cl_assert_equal_sz(424242, (size_t)result);
  cl_git_pass(git_thread_join(&thread2, &result));
	cl_assert_equal_sz(232323, (size_t)result);

  cl_assert_equal_i(2, *(test[0]));
  cl_assert_equal_i(2, *(test[1]));
#endif
}


#ifdef GIT_THREADS

static void *retrieve_storage(void)
{
	return (void *)0x1;
}

static void set_storage(void *payload)
{
	GIT_UNUSED(payload);
}

static void teardown_storage(void)
{
}

#endif

void test_threads_custom_tls__rejects_partial_callback_sets(void)
{
#ifndef GIT_THREADS
	clar__skip();
#else
	cl_git_fail(git_custom_tls_set_callbacks(retrieve_storage, NULL, teardown_storage));
	cl_git_fail(git_custom_tls_set_callbacks(NULL, set_storage, teardown_storage));
	cl_git_fail(git_custom_tls_set_callbacks(retrieve_storage, set_storage, NULL));

	cl_git_pass(git_custom_tls_set_callbacks(NULL, NULL, NULL));
	cl_git_pass(git_custom_tls_set_callbacks(retrieve_storage, set_storage, teardown_storage));
	cl_git_pass(git_custom_tls_set_callbacks(NULL, NULL, NULL));
#endif
}

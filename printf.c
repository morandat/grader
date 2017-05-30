#include <stdio.h>
#include <printf.h>
#include "grader.h"

#ifdef __APPLE__
printf_domain_t domain = NULL;

int register_printf_specifier(int spec, printf_function *render, printf_arginfo_function *arginfo) {
  if (!domain)
    domain = new_printf_domain();
  if (domain && register_printf_domain_function(domain, spec, render, arginfo, NULL)) {
    return (domain != NULL) + 1;
  }
  return 0;
}
static int printf_array_info(const struct printf_info *info, size_t n, int argtypes[]) {
  if (n > 1) {
    argtypes[0] = PA_INT /*| PA_FLAG_SIZE*/;
    argtypes[1] = PA_POINTER;
  }
  return 2;
}
#else
static int printf_array_info(const struct printf_info *info, size_t n, int argtypes[], int sizes[]) {
  (void) info;
  if (n > 1) {
    argtypes[0] = PA_INT /*| PA_FLAG_SIZE*/;
    sizes[0] = sizeof(int);
    argtypes[1] = PA_POINTER;
    sizes[1] = sizeof(void*);
  }
  return 2;
}
#endif

int printf_print_array(FILE *stream, const struct print_renderer *renderer, const struct printf_info *info, const void *const *args) {
  int len = 0;
  int values_count = *(int*)(args[0]); /* (size_t*) */
  const void * const *ptr = args[1];
  const char *fmt = renderer->get_format(info);
  if (ptr == NULL)
    return fprintf(stream, "NULL");
  if (!info->left)
    len += fprintf(stream, "{");
  for (int i = 0; i < values_count; i ++) {
    if (*ptr == NULL)
      len += fprintf(stream, "NULL");
    else
      len += renderer->render(stream, fmt + 2*(i == 0), *ptr);
    (*(char**)ptr) += renderer->type_size;
  }
  if (!info->left)
    len += fprintf(stream, "}");
  return len;
}

static const char* array_integer_fmt(const struct printf_info *info) {
  return (info->alt ? ", 0x%x" : (info->showsign ? ", %u" : ", %d"));
}
static int array_integer(FILE *stream, const char *fmt, const int * const *item) {
  return fprintf(stream, fmt, *item);
}
static struct print_renderer array_integer_renderer = {sizeof(int), array_integer_fmt, (int (*)(FILE *, const char* , const void *)) array_integer};
static int printf_print_array_integer(FILE *stream, const struct printf_info *info, const void *const* ptr) {
  return printf_print_array(stream, &array_integer_renderer, info, ptr);
}

static const char* array_pointer_fmt(const struct printf_info *info) {
  (void) info;
  return ", %p";
}
static int array_pointer(FILE *stream, const char *fmt, const void * const * item) {
  return fprintf(stream, fmt, *item);
}
static struct print_renderer array_pointer_renderer = {sizeof(void*), array_pointer_fmt, (int (*)(FILE *, const char* , const void *))array_pointer};
static int printf_print_array_pointer(FILE *stream, const struct printf_info *info, const void *const* ptr) {
  return printf_print_array(stream, &array_pointer_renderer, info, ptr);
}
static struct print_renderer _struct_renderer[] = {
  {sizeof(void*), array_pointer_fmt, (int (*)(FILE *, const char* , const void *))array_pointer},
  {sizeof(void*), array_pointer_fmt, (int (*)(FILE *, const char* , const void *))array_pointer}
};
static int printf_print_array_struct1(FILE *stream, const struct printf_info *info, const void *const* ptr) {
  return printf_print_array(stream, &_struct_renderer[0], info, ptr);
}
static int printf_print_array_struct2(FILE *stream, const struct printf_info *info, const void *const* ptr) {
  return printf_print_array(stream, &_struct_renderer[1], info, ptr);
}

static const char* array_string_fmt(const struct printf_info *info) {
  (void) info;
  return ", \"%s\"";
}
static int array_string(FILE *stream, const char * fmt, const char *** item) {
  return fprintf(stream, fmt, *item);
}
static struct print_renderer array_string_renderer = {sizeof(char*), array_string_fmt, (int (*)(FILE *, const char* , const void *))array_string};
static int printf_print_array_string(FILE *stream, const struct printf_info *info, const void *const* ptr) {
  return printf_print_array(stream, &array_string_renderer, info, ptr);
}

void struct_renderer(const struct print_renderer render) {
  _struct_renderer[0] = render;
}
void struct_renderer_alt(const struct print_renderer render) {
  _struct_renderer[1] = render;
}

int register_printf_handlers() {
  if (register_printf_specifier('I', printf_print_array_integer, printf_array_info) ||
      register_printf_specifier('D', printf_print_array_integer, printf_array_info) ||
      register_printf_specifier('P', printf_print_array_pointer, printf_array_info) ||
      register_printf_specifier('S', printf_print_array_string, printf_array_info)  ||
      register_printf_specifier('@', printf_print_array_struct1, printf_array_info) ||
      register_printf_specifier('[', printf_print_array_struct2, printf_array_info)) {
    perror("Registering printf conversions");
    return 1;
  }
  return 0;
}

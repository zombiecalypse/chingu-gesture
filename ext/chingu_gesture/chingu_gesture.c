#include <ruby.h>
#include <stdbool.h>
#include <limits.h>
#include <float.h>

const size_t INIT_BUFFER_SIZE = 255;
const size_t INIT_GESTURE_SIZE = 15;

typedef struct {
  unsigned int x, y;
} point_t;

typedef struct {
  unsigned int name;
  point_t* skeleton;
  size_t n_skeleton;
} gesture_t;

typedef struct {
  point_t* begin;
  point_t* current;
  size_t size;
  gesture_t* gestures;
  size_t n_gesture;
  size_t max_gesture;
} handler_t;

point_t make_point(int x, int y) {
  point_t p;
  p.x = x;
  p.y = y;
  return p;
}

void reset_handler(handler_t* handler) {
  handler->begin = NULL;
  handler->size = 0;
  handler->current = NULL;
  handler->n_gesture = 0;
  handler->gestures = NULL;
}

static void chingu_gesture_free(void* p) {
  unsigned int i;
  handler_t* handler = p;
  for (i = 0; i < handler->n_gesture; i++) {
    free(handler->gestures[i].skeleton);
  }
  free(handler->begin);
  free(handler->gestures);
  reset_handler(handler);
}

static VALUE chingu_gesture_alloc(VALUE klass) {
  handler_t* ptr;
  VALUE obj = Data_Make_Struct(klass, handler_t, NULL, chingu_gesture_free, ptr);

  reset_handler(ptr);
  return obj;
}

static VALUE chingu_gesture_init(VALUE self) {
  handler_t* handler;
  Data_Get_Struct(self, handler_t, handler);
  handler->begin = calloc(INIT_BUFFER_SIZE, sizeof(point_t));
  handler->size = INIT_BUFFER_SIZE;
  handler->current = handler->begin;
  handler->gestures = calloc(INIT_GESTURE_SIZE, sizeof(gesture_t));
  handler->n_gesture = 0;
  handler->max_gesture = INIT_GESTURE_SIZE;
  if (NULL == handler->begin) {
    rb_raise(rb_eNoMemError, "unable to allocate %ld bytes", INIT_BUFFER_SIZE*sizeof(point_t));
  }
  return self;
}

static void double_size(handler_t* handler) {
  size_t c = handler->current - handler->begin;
  point_t* new_points = calloc(2*handler->size, sizeof(point_t));
  memcpy(new_points, handler->begin, handler->size*sizeof(point_t));
  free(handler->begin);
  handler->begin = new_points;
  handler->size = 2*handler->size;
  handler->current = new_points + c;
}

static void double_size_gesture(handler_t* handler) {
  gesture_t* new_gestures = calloc(2*handler->max_gesture, sizeof(gesture_t));
  memcpy(new_gestures, handler->gestures, handler->max_gesture*sizeof(gesture_t));
  free(handler->gestures);
  handler->gestures = new_gestures;
  handler->max_gesture = 2*handler->max_gesture;
}

static VALUE chingu_gesture_add_point(VALUE self, VALUE x, VALUE y) {
  handler_t* handler;
  Data_Get_Struct(self, handler_t, handler);
  if (handler->current >=  handler->begin + handler->size) {
    double_size(handler);
  }
  *handler->current = make_point(FIX2UINT(x), FIX2UINT(y));
  handler->current++;
  return self;
}

static VALUE chingu_gesture_clear(VALUE self) {
  handler_t* handler;
  Data_Get_Struct(self, handler_t, handler);
  handler->current = handler->begin;
  return self;
}

static VALUE chingu_gesture_size(VALUE self) {
  handler_t* handler;
  Data_Get_Struct(self, handler_t, handler);
  return SIZET2NUM(handler->current - handler->begin);
}

static VALUE chingu_gesture_get_x(VALUE self, VALUE i) {
  handler_t* handler;
  size_t ii, csize;
  Data_Get_Struct(self, handler_t, handler);
  ii = NUM2SIZET(i);
  csize = handler->current - handler->begin;
  if (ii >= csize) {
    rb_raise(rb_eRuntimeError, "Out of range");
  }
  return UINT2NUM(handler->begin[ii].x);
}

static VALUE chingu_gesture_get_y(VALUE self, VALUE i) {
  handler_t* handler;
  size_t ii, csize;
  Data_Get_Struct(self, handler_t, handler);
  ii = NUM2SIZET(i);
  csize = handler->current - handler->begin;
  if (ii >= csize) {
    rb_raise(rb_eRuntimeError, "Out of range");
  }
  return UINT2NUM(handler->begin[ii].y);
}

static VALUE chingu_gesture_add_gesture(VALUE self, VALUE list) {
  handler_t* handler;
  gesture_t g;
  unsigned int len;
  unsigned int i;
  VALUE el;
  Data_Get_Struct(self, handler_t, handler);
  if (handler->n_gesture >= handler->max_gesture) {
    double_size_gesture(handler);
  }
  len = RARRAY_LEN(list);
  g.name = handler->n_gesture;
  g.skeleton = malloc(len*sizeof(point_t));
  g.n_skeleton = len;
  for (i = len-1; i < len; i--) {
    el = rb_ary_pop(list);
    g.skeleton[i].y = NUM2UINT(rb_ary_pop(el));
    g.skeleton[i].x = NUM2UINT(rb_ary_pop(el));
  }
  handler->gestures[g.name] = g;
  handler->n_gesture++;
  return UINT2NUM(g.name);
}

inline float max(float a, float b) {
  return a > b ? a : b;
}

inline float min(float a, float b) {
  return a < b ? a : b;
}

static void min_and_range(point_t* first, size_t n_first, float* min_first_x, float* min_first_y, float* first_range) {
  unsigned int i;
  float max_first_x = FLT_MIN, max_first_y = FLT_MIN;
  *min_first_x = FLT_MAX;
  *min_first_y = FLT_MAX;
  for (i = 0; i < n_first; i++) {
    max_first_x = max(max_first_x, first[i].x);
    max_first_y = max(max_first_y, first[i].y);
    *min_first_x = min(*min_first_x, first[i].x);
    *min_first_y = min(*min_first_y, first[i].y);
  }
  *first_range = max(max_first_x - (*min_first_x), max_first_y - (*min_first_y));
}

inline unsigned int index2d(unsigned int x, unsigned int y, size_t xm, size_t ym) {
  return x * ym + y;
}

inline float point_dist(point_t p1, point_t p2,
    float offset1_x, float offset1_y, float scale1,
    float offset2_x, float offset2_y, float scale2) {
  float x1 = (p1.x - offset1_x)/scale1;
  float y1 = (p1.y - offset1_y)/scale1;
  float x2 = (p2.x - offset2_x)/scale2;
  float y2 = (p2.y - offset2_y)/scale2;
  float dx = x1-x2;
  float dy = y1-y2;
  return dx*dx + dy*dy;
}

float sequence_dist(point_t* first, size_t n_first, point_t* second, size_t n_second) {
  // Align the sequences
  float min_first_x=0, min_first_y=0, first_range=0,
        min_second_x=0, min_second_y=0, second_range=0;
  float* table = malloc(sizeof(float) * n_first*n_second);
  unsigned int x, y;
  float d;
  min_and_range(first, n_first, &min_first_x, &min_first_y, &first_range);
  min_and_range(second, n_second, &min_second_x, &min_second_y, &second_range);
  // Dynamic time warping
  table[index2d(0,0, n_first, n_second)] = point_dist(first[0], second[0],
      min_first_x, min_first_y, first_range,
      min_second_x, min_second_y, second_range);
  for (x = 1; x < n_first; x++) {
    d = point_dist(first[x], second[0],
        min_first_x, min_first_y, first_range,
        min_second_x, min_second_y, second_range);
    table[index2d(x, 0, n_first, n_second)] = d + table[index2d(x-1, 0, n_first, n_second)];
  }
  for (y = 1; y < n_second; y++) {
    d = point_dist(first[0], second[y],
        min_first_x, min_first_y, first_range,
        min_second_x, min_second_y, second_range);
    table[index2d(0, y, n_first, n_second)] = d + table[index2d(0, y-1, n_first, n_second)];
  }
  for (x = 1; x < n_first; x++) {
    for (y = 1; y < n_second; y++) {
      d = point_dist(first[x], second[y],
          min_first_x, min_first_y, first_range,
          min_second_x, min_second_y, second_range);
      if (table[index2d(x-1, y, n_first, n_second)] < table[index2d(x,y-1, n_first,n_second)]) {
        table[index2d(x, y, n_first, n_second)] = d + table[index2d(x-1, y, n_first, n_second)];
      } else {
        table[index2d(x, y, n_first, n_second)] = d + table[index2d(x, y-1, n_first, n_second)];
      }
    }
  }

  
  d = table[index2d(n_first-1, n_second-1, n_first, n_second)];

  free(table);
  // Calculate average distance per point
  return d/(n_second * n_first);
}

static VALUE chingu_gesture_recognize(VALUE self) {
  handler_t* handler;
  unsigned int i;
  unsigned int min_name = 0;
  float min_dist = FLT_MAX;
  float cur_dist = FLT_MAX;
  Data_Get_Struct(self, handler_t, handler);
  if (handler->n_gesture == 0) {
    return Qnil;
  }
  for (i = 0; i < handler->n_gesture; i++) {
    cur_dist = sequence_dist(
        handler->gestures[i].skeleton, handler->gestures[i].n_skeleton,
        handler->begin, (size_t) (handler->current - handler->begin));
    if (cur_dist < min_dist) {
      min_dist = cur_dist;
      min_name = i;
    }
  }
  return rb_ary_new3(2, UINT2NUM(min_name), UINT2NUM(min_dist));
}

void Init_chingu_gesture(void) {
  VALUE klass;
  klass = rb_const_get(rb_cObject, rb_intern("ChinguGesture"));

  rb_define_alloc_func(klass, chingu_gesture_alloc);
  rb_define_method(klass, "initialize", chingu_gesture_init, 0);
  rb_define_method(klass, "add_point", chingu_gesture_add_point, 2);
  rb_define_method(klass, "clear", chingu_gesture_clear, 0);
  rb_define_method(klass, "size", chingu_gesture_size, 0);
  rb_define_private_method(klass, "_get_x", chingu_gesture_get_x, 1);
  rb_define_private_method(klass, "_get_y", chingu_gesture_get_y, 1);
  rb_define_private_method(klass, "_add_gesture", chingu_gesture_add_gesture, 1);
  rb_define_private_method(klass, "_recognize", chingu_gesture_recognize, 0);
}

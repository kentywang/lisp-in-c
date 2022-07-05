#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lisp.h"

static Element load_frame(const Binding *);

static Element the_empty_environment = {
  .type_tag = PAIR,
  .contents.pair_ptr = NULL
};

Boolean is_empty_environment(const Element env)
{
  return env.type_tag == the_empty_environment.type_tag &&
    env.contents.pair_ptr == the_empty_environment.contents.pair_ptr;
}

// Environment is a list of frame, where each frame is a list of bindings.
Element setup_environment(void)
{
  return make_cons(
    load_frame(initial_frame),
    the_empty_environment
  );
}

Binding find_binding(char *var, Element env)
{
  Binding b = {
    // This will be our signal to calling function indicating no binding
    // found.
    .variable = NULL
  };

  if (is_empty_environment(env))
    return b;

  do {
    Element frame_scanner = first_frame(env);

    // We still have pairs to scan through in frame.
    while (frame_scanner.contents.pair_ptr && strcmp(var, car(car(frame_scanner)).contents.symbol) != 0) {
      frame_scanner = cdr(frame_scanner);
    }

    if (frame_scanner.contents.pair_ptr) {
      // We exited the while loop because we found the variable.
      b.variable = var; // Use same string allocated as variable in parameter.
      b.value = cdr(car(frame_scanner));
      return b;
    }

    // Otherwise, not in this frame.
  } while (!is_empty_environment(env = enclosing_environment(env)));

  return b;
}

Element lookup_variable_value(char *var, Element env)
{
  printf("LOOKUP_VARIABLE_VALUE\n");
  Binding b = find_binding(var, env);

  if (b.variable)
    return b.value;

  fprintf(stderr, "Unbound variable: %s\n", var);
  exit(UNBOUND_VARIABLE);
}

/*
We'll be generating two pairs at a time, one pair as the backbone of the
list, and the other as the actual variable-and-value pair.

          full env
             /\
 this ______/  \   
 frame     /\   \
          /  \   \
         /\   \   \
        x  1  /\   \
             /\ \   \
            y  2 \   \
                nil  /\
                    /  \
                   /    \  
                next   nil
                frame
*/
Element load_frame(const Binding *b)
{
  Element frame_head = {
    .type_tag = PAIR,
    .contents.pair_ptr = NULL
  };

  // We need to handle the first element a bit differently since we set the
  // return element to point to the first backbone.
  if (b->variable) {
    Pair *curr_backbone = get_next_free_ptr();
    Pair *p = get_next_free_ptr();

    frame_head.contents.pair_ptr = curr_backbone;

    p->car.type_tag = SYMBOL;
    // We could also copy the string into GCed memory.
    p->car.contents.symbol = b->variable;
    p->cdr = b->value;

    // Wrapping Pair pointer in Element is optional, since the default
    // initialization gives it the PAIR type tag.
    curr_backbone->car.contents.pair_ptr = p;

    // Similar to above.
    while ((++b)->variable) { // Stop when we encounter END_OF_BINDINGS.
      curr_backbone->cdr.contents.pair_ptr = get_next_free_ptr();
      curr_backbone = curr_backbone->cdr.contents.pair_ptr;

      p = get_next_free_ptr();
      p->car.type_tag = SYMBOL;
      p->car.contents.symbol = b->variable;
      p->cdr = b->value;

      curr_backbone->car.contents.pair_ptr = p;
    }
  }

  return frame_head;
}

// (a b c) (1 2 3) => ((a . 1) (b . 2) (c . 3))
Element make_frame(const Element bindings, const Element values)
{
  // If only one of bindings or values is null, we have a parameter-argument
  // mismatch. Hopefully !s works.
  // TODO: List procedure name, number of args vs. parameters
  if (!bindings.contents.pair_ptr != !values.contents.pair_ptr) {
    fprintf(stderr, "Arity mismatch during make_frame.\n");
    exit(ARITY_MISMATCH);
  }

  // Both empty.
  if (!bindings.contents.pair_ptr && !values.contents.pair_ptr) {
    Element e = {
      .type_tag = PAIR,
      .contents.pair_ptr = NULL
    };

    return e;
  }

  //X printf("make_frame\n");
  // print_element(bindings);
  //X printf("\n");
  // Still have parameters and arguments.
  //X printf("bindings addr: %p\n", &bindings);
  car(bindings);
  //X printf("car bindings done\n");
  cdr(bindings);
  //X printf("cdr bindings done\n");
  car(values);
  //X printf("car values done\n");
  cdr(values);
  //X printf("cdr values done\n");
  return make_cons(
    make_cons(car(bindings), car(values)),
    make_frame(cdr(bindings), cdr(values))
  );
}

Element first_frame(const Element env)
{
  return car(env);
}

Element enclosing_environment(const Element env)
{
  return cdr(env);
}
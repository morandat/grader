# Should be one of the malloc implementation available (counter/sentinel/protector)
CFLAGS=-std=c99 -Wall
MALLOC = counter

OVERRIDEN_FUNCTIONS=-Dmain=alt_main \
					-D__assert_fail=grader_assert_fail \
					-D__assert_perror_fail=grader_assert_perror_fail \
					-Dexit=grader_exit

STUDENT_CODE_CFLAGS=-finstrument-functions -DSTUDENT_MODE

-include Makefile.mk

CFLAGS+=-I../lib -I. -fno-builtin-printf
LDFLAGS+=-ldl -rdynamic

GRADER= ../lib/grader.o ../lib/printf.o ../lib/malloc-$(MALLOC).o

DEBUG_FLAGS=-ggdb -O0
CFLAGS_EXO+=-Wno-address -Wno-format -Wno-missing-field-initializers

ifndef ILLEGAL
ILLEGAL=assert_passed
endif

ILLEGAL_PAT=($(strip $(ILLEGAL))|_exit|assert_passed|reset_user_assert)

%: %.c libexam.a
		(\
			echo '#line 1 "prolog.c"' ; \
			ar p libexam.a prolog.c 2>/dev/null ;\
			echo '#line 1 "$(notdir $<)"' ; \
			cat $<  ;\
			echo '#line 1 "epilog.c"' ; \
			ar p libexam.a epilog.c 2>/dev/null \
		) | $(CC) -xc $(CFLAGS) -c -o $@.o $(DEBUG_FLAGS) $(OVERRIDEN_FUNCTIONS) $(STUDENT_CODE_CFLAGS) -
		nm -u $@.o 2>/dev/null | egrep -q "[Uu] \b$(ILLEGAL_PAT)\b" && echo "Veuillez ne pas utiliser, appeller ou créer, de fonction dont le nom est :" `nm $@.o 2>/dev/null | egrep -o "\b$(ILLEGAL_PAT)\b"` && exit 1 || true
	$(CC) -o $@ $@.o libexam.a $(LDFLAGS)
exercice.o: exercice.c $(DEPS)
	$(CC) -c -o $@ $(DEBUG_FLAGS) $(CFLAGS_EXO) $(CFLAGS) $<
libexam.a: exercice.o $(OBJS) $(GRADER)
	$(AR) $(ARFLAGS) -c $@ $^ > /dev/null
enonce.c: provided.c
	../lib/extract_subject $< > $@
provided.h: provided.c
	../lib/extract_subject --provided $< > $@
provided.o: provided.c
	$(CC) -c -o $(patsubst %.o,%.tmp.o,$@) $(DEBUG_FLAGS) $(CFLAGS_EXO) $(CFLAGS) $<
	../lib/redefine_syms $(patsubst %.o,%.tmp.o,$@) $@
clean:
	$(RM) $(GRADER) $(DEPS)

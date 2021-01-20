CC ?= gcc
CFLAGS = -std=c99 -Os -Wall -Wextra
JAVAC = javac
PATCH = --patch-module java.base=java

BIN = jvm
<<<<<<< HEAD
OBJ = jvm.o stack.o jvm_info.o class_heap.o object_heap.o
=======
OBJ = jvm.o stack.o jvm_info.o class_heap.o object_heap.o native.o
JAVA = target
>>>>>>> 0c9995f... Implement native java class to let invokevirtual more resonable

include mk/common.mk
include mk/jdk.mk

# Build PitifulVM
all: target $(BIN)
$(BIN): $(OBJ)
	$(VECHO) "  CC+LD\t\t$@\n"
	$(Q)$(CC) -o $@ $^

%.o: %.c
	$(Q)$(CC) $(CFLAGS) -c -o $@ $<

target:
	$(Q)$(JAVAC) $(PATCH) java/*/*.java

TESTS = \
	Factorial \
	Return \
	Constants \
	MoreLocals \
	PrintLargeNumbers \
	Collatz \
	PythagoreanTriplet \
	Arithmetic \
	CoinSums \
	DigitPermutations \
	FunctionCall \
	Goldbach \
	IntegerTypes \
	Jumps \
	PalindromeProduct \
	Primes \
	Recursion \
	Constructor \
	InvokeVirtual \
	Static \
	Array \
	Strings \
	Switch

check: target $(addprefix tests/,$(TESTS:=-result.out)) 

tests/%.class: tests/%.java
	$(Q)$(JAVAC) $^

tests/%-expected.out: tests/%.class
	$(Q)$(JAVA) -cp tests $(*F) > $@

tests/%-actual.out: tests/%.class jvm
	$(Q)./jvm $< > $@

tests/%-result.out: tests/%-expected.out tests/%-actual.out
	$(Q)diff -u $^ | tee $@; \
	name='test $(@F:-result.out=)'; \
	$(PRINTF) "Running $$name..."; \
	if [ -s $@ ]; then $(PRINTF) FAILED $$name. Aborting.; false; \
	else $(call pass); fi

clean:
	$(Q)$(RM) *.o jvm tests/*.out tests/*.class java/*/*.class $(REDIR)

.PRECIOUS: %.o tests/%.class tests/%-expected.out tests/%-actual.out tests/%-result.out

indent:
	clang-format -i *.c *.h 
	cloc jvm.c

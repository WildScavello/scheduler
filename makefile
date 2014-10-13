CFLAGS=-std=gnu99 -g
LINKLIBS= -pthread -lm
OBJ_FILES=project1.o scheduler.o

a.out: $(OBJ_FILES)
	gcc -o a.out $(OBJ_FILES) $(LINKLIBS)

clean:
	rm $(OBJ_FILES)
test: test-fcfs test-srtf test-pbs test-mlfq

test-fcfs:
	./a.out 0 p1_test/TestInputs/input_1 | diff p1_test/TestOutputs/base_input_1_fcfs_output - || echo 'mismatch'
	./a.out 0 p1_test/TestInputs/input_2 | diff p1_test/TestOutputs/base_input_2_fcfs_output - || echo 'mismatch'
	./a.out 0 p1_test/TestInputs/input_3 | diff p1_test/TestOutputs/base_input_3_fcfs_output - || echo 'mismatch'
test-srtf:
	./a.out 1 p1_test/TestInputs/input_1 | diff p1_test/TestOutputs/base_input_1_srtf_output - || echo 'mismatch'
	./a.out 1 p1_test/TestInputs/input_2 | diff p1_test/TestOutputs/base_input_2_srtf_output - || echo 'mismatch'
	./a.out 1 p1_test/TestInputs/input_3 | diff p1_test/TestOutputs/base_input_3_srtf_output - || echo 'mismatch'
test-pbs:
	./a.out 2 p1_test/TestInputs/input_1 | diff p1_test/TestOutputs/base_input_1_pbs_output - || echo 'mismatch'
	./a.out 2 p1_test/TestInputs/input_2 | diff p1_test/TestOutputs/base_input_2_pbs_output - || echo 'mismatch'
	./a.out 2 p1_test/TestInputs/input_3 | diff p1_test/TestOutputs/base_input_3_pbs_output - || echo 'mismatch'
test-mlfq:
	./a.out 3 p1_test/TestInputs/input_1 | diff p1_test/TestOutputs/base_input_1_mlfq_output - || echo 'mismatch'
	./a.out 3 p1_test/TestInputs/input_2 | diff p1_test/TestOutputs/base_input_2_mlfq_output - || echo 'mismatch'
	./a.out 3 p1_test/TestInputs/input_3 | diff p1_test/TestOutputs/base_input_3_mlfq_output - || echo 'mismatch'

package:
	tar czf 'Burlew-Seamus.tgz' scheduler.c overview.txt

project1.o: project1.c
	gcc $(CFLAGS) -c $<
scheduler.o: scheduler.c
	gcc $(CFLAGS) -c $<


default:
  gcc -O4 clockout.c -lncursesw -ljson-c -o clockout

clean:
  rm -f *.o
  rm -f clockout

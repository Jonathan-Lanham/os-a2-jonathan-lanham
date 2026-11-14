#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "kernel/pinfo.h"
#include "user/user.h"

// Convert enum procstate integer to readable string
char *
state_name(int state)
{
  switch(state) {
    case 0: return "UNUSED";
    case 1: return "USED";
    case 2: return "SLEEPING";
    case 3: return "RUNNABLE";
    case 4: return "RUNNING";
    case 5: return "ZOMBIE";
    default: return "UNKNOWN";
  }
}

void
print_table(char *label)
{
  struct pinfo procs[NPROC];
  int n = getpinfo(procs, NPROC);

  printf("\n=== %s (active: %d) ===\n", label, n);
  printf("PID\tSTATE\t\tLEVEL\tTICKS\tNSCHED\tCTIME\tNAME\n");
  for(int i = 0; i < n; i++) {
    printf("%d\t%s\t%s%d\t%d\t%d\t%d\t%s\n",
           procs[i].pid,
           state_name(procs[i].state),
           (procs[i].state == 1) ? "\t" : "", // Tab alignment for "USED"
           procs[i].level,
           procs[i].runticks,
           procs[i].nsched,
           procs[i].ctime,
           procs[i].name);
  }
}

// Busy loop to burn CPU ticks
void
cpu_work(int iterations)
{
  int x = 0;
  for(int i = 0; i < iterations; i++) {
    for(int j = 0; j < 1000000; j++) {
      x += j;
    }
  }
  asm volatile("" : : "r"(x));
}

// Test 1: Validate getpinfo(buf, max) argument handling
void
test_getpinfo_bounds(void)
{
  struct pinfo small[2];

  printf("\n--- Test 1: getpinfo() Edge Cases ---\n");

  if(getpinfo(small, 0) == -1)
    printf("[PASS] max = 0 returned -1\n");
  else
    printf("[FAIL] max = 0 did not fail\n");

  if(getpinfo(small, -5) == -1)
    printf("[PASS] max = -5 returned -1\n");
  else
    printf("[FAIL] max = -5 did not fail\n");

  if(getpinfo((struct pinfo *)0, 5) == -1)
    printf("[PASS] null pointer returned -1\n");
  else
    printf("[FAIL] null pointer did not fail\n");

  int n = getpinfo(small, 2);
  if(n == 2)
    printf("[PASS] max = 2 capped output to %d processes\n", n);
  else
    printf("[FAIL] max = 2 returned %d\n", n);
}

// Test 2: CPU-bound vs I/O-bound priority behavior
void
test_mlfq_demotion(void)
{
  printf("\n--- Test 2: CPU-Bound Demotion vs I/O-Bound Retention ---\n");

  int pid_cpu = fork();
  if(pid_cpu == 0) {
    cpu_work(80); // Burn enough ticks to drop from level 3 to level 0
    exit(0);
  }

  int pid_io = fork();
  if(pid_io == 0) {
    // Frequently yield via sleep(1) so time_slice never expires
    for(int i = 0; i < 15; i++) {
      pause(1); // Use sleep(1) if your xv6 version uses sleep() instead of pause()
    }
    exit(0);
  }

  // Let children run briefly before inspecting
  pause(10); // Replace with sleep(10) on older xv6-riscv commits
  print_table("Mid-Run Snapshot (CPU child should be low level, IO child level 3)");

  wait(0);
  wait(0);
}

// Test 3: Starvation prevention (aging boost)
void
test_starvation_boost(void)
{
  printf("\n--- Test 3: Starvation Prevention (Aging) ---\n");

  // Spawn 2 CPU hogs that drop to level 0 and compete
  int p1 = fork();
  if(p1 == 0) {
    cpu_work(150);
    exit(0);
  }

  int p2 = fork();
  if(p2 == 0) {
    cpu_work(150);
    exit(0);
  }

  // Parent sleeps long enough for waiting RUNNABLE child to age and boost
  pause(50); // Replace with sleep(50) on older xv6-riscv commits
  print_table("Aging Snapshot (Check if waiting CPU hog boosted above level 0)");

  wait(0);
  wait(0);
}

int
main(int argc, char *argv[])
{
  printf("Starting MLFQ & getpinfo test suite...\n");

  test_getpinfo_bounds();
  test_mlfq_demotion();
  test_starvation_boost();

  printf("\nAll tests completed.\n");
  exit(0);
}

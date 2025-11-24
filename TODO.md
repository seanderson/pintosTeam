# Primary list of project tasks organized by date in descending order.

## Sat 15 Nov 2025

- [ ] Determine why the program hangs; killed by 10 sec timeout. AS MP
- [ ] Review the existing argument parsing and determine whether it works for user programs having multiple arguments.  Indicate problems and propose solutions for this.  ZA
- [ ] Pass the args-none test (created by typing *make tests/userprog/args-none.result* in userprog/build. TT
- [ ] Determine how best to access user memory (manual p. 27).  Validating user addresses. HA
- [ ] Implement exit syscall.  What happens when a program ends? SK
- [ ] SK:
- [ ]   - Currently, process_exit clears the pointer to the page dir used by the current process
- [ ]   - and then it switches to the kernel page dir
- [ ]   - and then destroys the page dir used by the current thread
- [ ]     ----------------
- [ ]   - This on its own is an issue because a proper process exit requires much more:
- [ ]   - closing all open files including the executable
- [ ]   - notifying the parent process and storing the exit status
- [ ]   - handling orphaned children
- [ ]     ----------------
- [ ]   - right now, I don't know how to see exactly what happens when process_exit is called but i do see the issues as listed above
- [ ] Syscall write needs to work. KK

## Future

- [ ] Pass args-single
- [ ] Pass args-multiple
- [ ] Pass args-many
- [ ] Pass args-dbl-space


In this assignment we will try to make a *speculative execution attack* that abuses some weird processor optimization, paired with some virtual memory shenanigans.

### Store/load forwarding
When you'd like to load something from some address A, the processor would check the ROB for a recent store to this address. If there is one, it would forward the data that was about to be stored into whatever is asking for the load.  

This might have a lot of nuances, given how the ROB would order the instructions, how it would schedule them, how close do they have to be, all assuming we're in an OOOE scheme (?) and many more things I'm not noticing.

----------
### Biography of Biggus Dickus

> This section says that if we have two virtual addresses A and B such that A = B mod 4096 (referred to as 4K aliasing) then a load to A that appears shortly in the code after a store to B has a performance penalty.

Biggus Dickus looked at [section 15.8](https://software.intel.com/sites/default/files/managed/9e/bc/64-ia-32-architectures-optimization-manual.pdf); The people who wrote the manual seems to have observed some time penalty when trying to load into a virtual address after storing into another address that is 4k aligned to it.

> Biggus hypothesizes that this occurs because of speculative store/load forwarding

> but store/load forwarding can occur only for accesses to the same physical address

Biggus seems to have tried to find a single arrow of causation between the time penalty and both speculative execution and this store/loading forwarding optimization.

On a 4K virtual page size system, the last 12 bits of a virtual address seem to be used for choosing an offset within a page. This '4k alignment' refers to the last 12 bits of two addresses matching.

Biggus thinks that given that it seems to take time for a virtual address to get translated, if a sequence of instructions has a load closely followed by a store, and both of these addresses 4K align, the processor would attempt to **speculate** and forward the store value into the load.

```
store(B)
..
load(A) // this gets store(B) 
..
```

Then, if the speculation was 'wrong', as in, the virtual address for the load gets fully translated and it's different than the physical address of B, a roll-back happens. 

I'm not sure how the roll-backs squash and re-execute things and what gets re-executed, but Biggus seems to think that the load gets executed again and that causes the time penalty. 

----------

> This kind of speculation can be exploited in a same-thread attack, for example in the context of JavaScript or a similar VM. Suppose the VM writes to address A, which is secret and cannot be accessed by untrusted code. But if the untrusted code has a load to address B that 4K aliases with A, it will get the contents of the store through speculative forwarding  and can hopefully leak it through a cache side-channel before being squashed

We seem to want to try and abuse this squashing and this weird 'easy access to addresses' using 4k alignment. If the victim were to write something into an address, and we would load into an address that is 4K aligned with it, hopefully we could get the processor to speculate, if Biggus's assumption was right.

Then as we have the data we could try to compose it with a gadget that leaks information outside, before everything gets squashed.

This is all quite high-level, I'm not sure I follow. 

```
store(A) 
..
// squash zone
	load(B)
	leak(B)
// 
```

### Tips

> Read and understand spectre.c to see how to flush cache lines and leak bytes over a cache side-channel. You can then modify the file to implement Herbert’s attack.  


> You can use the Linux tool perf to monitor CPU performance counter events of your program. You can read about perf here: http://www.brendangregg.com/perf.html#CPUstatistics.  

> For example, on the TAU rack-mad-03 machine, you can collect statistics on the 4K aliasing events by using the following command:

```
perf stat -e cpu/event=0x7,umask=0x1,name=ld_blocks_partial_address_alias/
```
.section .init
   /* gcc will nicely put the contents of crtend.o's .init section here. */
   pop %ebp
   ret
/*.size _init, . - _init*/
 
.section .fini
   /* gcc will nicely put the contents of crtend.o's .fini section here. */
   pop %ebp
   ret
/*.size _fini, . - _fini*/

rm -f ./.obj/romboot.cmd
cpp -I. -ansi -D__ASSEMBLY__ -MD -MT ./.obj/romboot.d  -E ./romboot.cmd  -o ./.obj/romboot.cmd



src=$(wildcard *.c)
obj=$(patsubst %.c,%.o,$(src))

target=out

ALL:$(target)

$(target):$(obj)
	gcc $^ -o $@
$(obj):%.o:%.c
	gcc -c $< -o $@
clean:
	-rm -rf $(obj) $(target)

.PHONY:clean ALL

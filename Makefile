librsocket.a: rsocket.h rsocket.c
	@gcc -c rsocket.c -lpthread
	@ar -crs $@ rsocket.o
	@rm rsocket.o

user1: user1.c librsocket.a
	@gcc $^ -o $@ -lpthread

user2: user2.c librsocket.a
	@gcc $^ -o $@ -lpthread

run1: user1
	@./user1

run2: user2
	@./user2

clean:
	@rm -rf user1 user2 librsocket.a rsocket.o

# t2: t2.c
# 	@gcc t2.c -o t2

# t1: t1.c
# 	@gcc t1.c -o t1

# test2: t2
# 	@./t2

# test1: t1
# 	@./t1



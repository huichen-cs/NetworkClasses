all: \
	udp_hello_srv udp_hello_cli udp_zero_cli udp_file_cli \
	tcp_hello_srv tcp_hello_cli tcp_zero_cli tcp_file_cli \
	msgsrv msgcli

CC := gcc

ifeq ($(BIND_TO_ANY),1)
	MACROS := -DBIND_TO_ANY
endif

ifneq ($(SERVER_IP),"")
	MACROS := $(MACROS) -DSERVER_IP=\"$(SERVER_IP)\"
endif

CFLAGS := -g -Wall $(MACROS)

udp_hello_srv: udp_hello_srv.o
	$(CC) -o udp_hello_srv udp_hello_srv.o

udp_hello_cli: udp_hello_cli.o
	$(CC) -o udp_hello_cli udp_hello_cli.o

udp_zero_cli: udp_zero_cli.o
	$(CC) -o udp_zero_cli udp_zero_cli.o

udp_file_cli: udp_file_cli.o
	$(CC) -o udp_file_cli udp_file_cli.o

tcp_hello_srv: tcp_hello_srv.o
	$(CC) -o tcp_hello_srv tcp_hello_srv.o

tcp_hello_cli: tcp_hello_cli.o
	$(CC) -o tcp_hello_cli tcp_hello_cli.o

tcp_zero_cli: tcp_zero_cli.o
	$(CC) -o tcp_zero_cli tcp_zero_cli.o

tcp_file_cli: tcp_file_cli.o
	$(CC) -o tcp_file_cli tcp_file_cli.o

msgsrv: msgsrv.o
	$(CC) -o msgsrv msgsrv.o -lncurses

msgcli: msgcli.o
	$(CC) -o msgcli msgcli.o -lncurses

clean:
	$(RM) *.o \
		udp_hello_srv udp_hello_cli udp_zero_cli udp_file_cli	\
		tcp_hello_srv tcp_hello_cli tcp_zero_cli tcp_file_cli	\
		msgsrv msgcli
		





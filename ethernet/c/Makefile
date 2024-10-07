# Copyright (C) 2015 Hui Chen <huichen AT ieee DOT org>
# 
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

all: ethercap etherinj ethersend etherrecv 

CFLAGS=-Wall -Wextra

ethersend: ethersend.o buffer.o
	$(CC) $(LDFLAGS) ethersend.o buffer.o -o ethersend

etherrecv: etherrecv.o buffer.o sighandler.o
	$(CC) $(LDFLAGS) etherrecv.o buffer.o sighandler.o -o etherrecv

ethercap: ethercap.o buffer.o sighandler.o
	$(CC) $(LDFLAGS) ethercap.o buffer.o sighandler.o -o ethercap

etherinj: etherinj.o buffer.o sighandler.o
	$(CC) $(LDFLAGS) etherinj.o buffer.o sighandler.o -o etherinj

clean:
	$(RM) *.o ethercap etherinj etherrecv ethersend

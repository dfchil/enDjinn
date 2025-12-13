TARGET:=bin/font_enj

all: ${TARGET}

enj_bitmap.o: ../../code/enj_bitmap.c
	gcc -std=gnu23 -Og -g -c $< -o $@ -I ../../include

${TARGET}: $(basename $(notdir ${TARGET})).c ../stb/stb_image_write.h ../stb/stb_truetype.h enj_bitmap.o
	@mkdir -p bin/
	gcc -L/opt/homebrew/lib/  enj_bitmap.o -std=gnu23 -Og -g  $< -o $@ -lm 

clean:
	rm -f ${TARGET} *.o
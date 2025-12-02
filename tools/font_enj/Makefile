TARGET: font_enj

all: ${TARGET}

${TARGET}: $(basename ${TARGET}).c ../stb/stb_image_write.h ../stb/stb_truetype.h
	gcc -std=gnu23 -Og -g $< -o $@ -lm

clean:
	rm -f ${TARGET}
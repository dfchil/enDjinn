TARGET:=bin/font_enj

all: ${TARGET}

${TARGET}: $(basename $(notdir ${TARGET})).c ../stb/stb_image_write.h ../stb/stb_truetype.h
	@mkdir -p bin/
	gcc -L/opt/homebrew/lib/ -std=gnu23 -Og -g $< -o $@ -lm 

clean:
	rm -f ${TARGET}
all:
	@echo "Build-It-Up"
	mkdir -p build/obj
	mkdir -p build/bin

	@echo "Making dummy UI"
	gcc -std=gnu99 -I"./include" src/control.c src/control_ui.c -o build/bin/ui

	@echo "Making engine endpoint"

clean:
	rm -rf build/

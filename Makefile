all:
	@echo "Build-It-Up"
	mkdir -p build/obj
	mkdir -p build/bin

	@echo "Making dummy UI"
	gcc -std=gnu99 -I"./include" src/mgmt.c src/control_ui.c -o build/bin/ui

	@echo "Making engine endpoint"
	gcc -std=gnu99 -I"./include" src/mgmt.c src/engine.c -o build/bin/engine

	@echo "***************"
	@echo "Make successful"
	@echo "***************"

clean:
	rm -rf build/

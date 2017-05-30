ifndef DST
    $(error DST should be defined)
endif
EXAM=$(DST)/exam
LIB=$(EXAM)/lib

syntax:
	echo Exemple: make -f exam/Makefile DST=bar remove lib leap_year.c even.c finish
	false
remove:
	$(RM) -r $(DST)
lib:
	@echo Populating $(LIB)
	mkdir -p $(LIB)
	cp lib/Makefile $(LIB)
	cp lib/*.c $(LIB)
	cp lib/*.h $(LIB)
	cp lib/extract_subject $(LIB)
	cp lib/redefine_syms $(LIB)
	cp exam/Makefile.base $(DST)/Makefile
	cp exam/instructions.txt $(DST)/
	cp exam/Makefile.release $(DST)/
	cp exam/gdbinit $(DST)/.gdbinit
finish:
	echo Fixing targets: $(basename $(notdir $(wildcard $(DST)/*.c)))
	sed -i.bak -e 's:@TARGETS@:$(basename $(notdir $(wildcard $(DST)/*.c))):' $(DST)/Makefile
	$(RM) $(DST)/Makefile.bak
	echo "To release, use on the correct architecture:"
	echo "\tmake -f Makefile.release release"
%/provided.h: %/provided.c %/prolog.c %/epilog.c
	lib/extract_subject --provided $< > $@
%/enonce.c: %/provided.c
	lib/extract_subject $< > $@
%.c: %/enonce.c %/exercice.c %/Makefile.mk
	@test -d $(LIB) || (echo use the rule lib first && false)
	@echo Adding exercice $(basename $@)
	@mkdir -p $(EXAM)/$(dir $<)
	@cp $< $(DST)/$@
	echo $(basename $@)
#un peu brutal
	@cp $(dir $<)*.[c,h] $(dir $<)Makefile.mk $(EXAM)/$(dir $<) 
	@echo $(basename $@): $@ >> $(DST)/Makefile
	@echo '\t@echo Compiling: $@' >> $(DST)/Makefile
	@echo '\t@make -sC exam/$(basename $@) -f ../lib/Makefile ../../$(basename $@)' >> $(DST)/Makefile

.PHONY: syntax remove lib finish
.SILENT: lib syntax finish

# Makefile wrapper (emacs rules !)
# by benjamin gerard <ben@sashipa.com>
#
# $Id$
#

%: force
	@echo "Wrap target [$@]"
	@$(MAKE) -C ../src $(filter-out,Makefile,$@)
force: ;

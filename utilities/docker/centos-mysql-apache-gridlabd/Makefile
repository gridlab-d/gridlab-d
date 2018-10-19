# configuration variables
include Makefile.conf

# automatically constructed variables
GRIDLABD_IMAGE=$(GRIDLABD_IMG):$(GRIDLABD_TAG)
MYSQLD_IMAGE=$(MYSQLD_IMG):$(MYSQLD_TAG)
HTTPD_IMAGE=$(HTTPD_IMG):$(HTTPD_TAG)

help:
	@echo "Valid targets:"
	@echo " "
	@echo "  General targets"
	@echo "  ---------------"
	@echo "  install          pull and starts all the required docker images"
	@echo "  status           lists the status of the docker services"
	@echo "  start            starts all the required docker services"
	@echo "  stop             stops all the required docker services"
	@echo "  restart          restart all the required docker services"
	@echo "  clean            cleans up all docker images"
	@echo "  help             displays this list (default)"
	@echo " "
	@echo "  GridLAB-D targets"
	@echo "  -----------------"
	@echo "  install-gridlabd pulls and start the gridlabd image from $(GRIDLABD_IMAGE)"
	@echo "  pull-gridlabd    pulls the gridlabd image from $(GRIDLABD_IMAGE)"
	@echo "  start-gridlabd   starts the gridlabd server, mysqld, and httpd services"
	@echo "  stop-gridlabd    stops the gridlabd server, mysqld, and httpd services"
	@echo "  build-gridlabd   creates the gridlabd image on local disk from $(GRIDLABD_IMAGE)"
	@echo "  validate         run a validation test of the current install"
	@echo "  push-gridlabd    pushes the gridlabd image to the docker repository"
	@echo "  clean-gridlabd   cleans up gridlabd docker images"
	@echo " "
	@echo "  MySQL targets"
	@echo "  -------------"
	@echo "  install-mysqld   pulls and starts the mysqld image from $(MYSQLD_IMAGE)"
	@echo "  pull-mysqld      pulls the mysqld image from $(MYSQLD_IMAGE)"
	@echo "  start-mysqld     starts the mysqld image from $(MYSQLD_IMAGE)"
	@echo "  stop-mysqld      stop the mysqld image from $(MYSQLD_IMAGE)"
	@echo "  restart-mysqld   restarts the mysqld image from $(MYSQLD_IMAGE)"
	@echo "  clean-mysqld     cleans up mysqld docker images"
	@echo " "
	@echo "  Apache targets"
	@echo "  --------------"
	@echo "  install-httpd    pulls and starts the httpd image from $(HTTPD_IMAGE)"
	@echo "  pull-httpd       pulls the httpd image from $(HTTPD_IMAGE)"
	@echo "  start-httpd      starts the httpd image from $(HTTPD_IMAGE)"
	@echo "  stop-httpd       stop the httpd image from $(HTTPD_IMAGE)"
	@echo "  restart-httpd    restarts the httpd image from $(HTTPD_IMAGE)"
	@echo "  clean-httpd      cleans up httpd docker images"
	@echo " "

install: gridlabd mysqld httpd

status:
	@echo "SERVICE    STATUS"
	@echo "---------- -----------------------"
	@echo "gridlabd   $$(docker inspect -f '{{.State.Health.Status}}' gld4 2>/dev/null || echo 'not started')" | tr -d '\n' ; echo ""
	@echo "httpd      $$(docker inspect -f '{{.State.Health.Status}}' apache2 2>/dev/null || echo 'not started')" | tr -d '\n' ; echo ""
	@echo "mysqld     $$(docker inspect -f '{{.State.Health.Status}}' mysql56 2>/dev/null || echo 'not started')" | tr -d '\n' ; echo ""

start: 
	# TODO start the services

stop: 
	# TODO stop the services

restart: stop start #

clean: clean-gridlabd clean-mysqld clean-httpd
	docker system prune -f

clean-mysqld: 
	@# clean up running image
	docker kill mysqld mysql56 2>/dev/null || true
	docker rm mysqld 2>/dev/null || true
	docker rmi -f $(MYSQLD_IMAGE) 2>/dev/null || true
	rm -rf mysqld
	docker system prune -f

clean-httpd:
	@# clean up running image
	docker kill httpd php 2>/dev/null || true
	docker rm httpd php 2>/dev/null || true
	docker rmi -f $(HTTPD_IMAGE) 2>/dev/null || true
	docker system prune -f

clean-gridlabd: stop-mysqld
	@# clean up running image
	docker rm gridlabd 2>/dev/null || true
	docker rmi -f $(GRIDLABD_IMAGE) gridlabd 2>/dev/null || true
	rm -f gridlabd.img dockerfile.tmp
	docker system prune -f
	
gridlabd.img: DockerFile system.sh gridlabd.sh Makefile.conf
	@# build a new image from scripts
	@[ ! -f dockerfile.tmp ] || ( echo "ERROR: a build is already running or was stopped before it finished (dockerfile.tmp exists--try 'make clean')."; fail)
	./make_dockerfile DockerFile Makefile.conf dockerfile.tmp
	docker build --no-cache -f dockerfile.tmp -t $(GRIDLABD_IMAGE) . && docker save $(GRIDLABD_IMAGE) -o gridlabd.img ; rm -f dockerfile.tmp
	@[ -f gridlabd.img ] || ( echo "ERROR: build failed--cleaning up image"; docker rmi -f $$(docker images -q | head -n 1) )

build: gridlabd.img 

push: build 
	docker push $(GRIDLABD_IMAGE)

gridlabd: Makefile.conf
	@# pull image
	docker pull $(GRIDLABD_IMAGE)
	@# create command alias in .profile_bash
	grep 'alias gridlabd' $(HOME)/.bash_profile || echo "alias gridlabd='docker run -it -v $(pwd):/gridlabd gridlabd gridlabd -W /gridlabd'" >>$(HOME)/.bash_profile
	docker tag $(GRIDLABD_IMAGE) gridlabd
	# setup gridlabd simulation
	# TODO
	# start gridlabd simulation
	# TODO

mysqld: pull-mysqld start-mysqld

restart-mysqld: stop-mysqld start-mysqld

stop-mysqld: Makefile.conf
	docker rm -f mysqld mysql56 1>/dev/null 2>&1 || true

pull-mysqld:
	@docker rmi -f $(MYSQLD_IMAGE) 1>/dev/null 2>&1 || true
	@docker system prune -f 1>/dev/null 2>&1 || true
	docker pull $(MYSQLD_IMAGE)

start-mysqld: Makefile.conf
	@# pull image
	@test "$$(docker inspect -f '{{.State.Health.Status}}' mysql56 2>&1)" != "healthy" || ( echo "mysql56 is already running ('make stop-mysqld' will kill it)"; false )
	@test -z "$$(netstat -vatn | grep -w 3306 | grep -w LISTEN)" || ( echo "ERROR: cannot start mysqld because another service is listening on port 3306" ; false )
	@# start service
	docker run --name=mysql56 -p 3306:3306 -e MYSQL_ROOT_PASSWORD=$(MYSQL_ROOT_PASSWORD) -d $(MYSQLD_IMAGE)
	@echo "Waiting for mysql to start..."
	@while [ "$$(docker inspect -f '{{.State.Health.Status}}' mysql56)" == "starting" ]; do sleep 1; done
	docker logs mysql56
	@test "$$(docker inspect -f '{{.State.Health.Status}}' mysql56)" == "healthy"
	@echo "mysqld started ok."
	@# run setup script
	docker exec -i mysql56 mysql -uroot -pgridlabd < mysql-setup.sql || ( docker rm -f mysqld mysql56 1>/dev/null 2>&1 && fail )

httpd: Makefile.conf
	# pull images
	docker images | grep '^httpd' || docker pull httpd
	docker images | grep '^php' || docker pull php
	# compose the docker image
	# TODO
	# start httpd service
	# TODO

validate: gridlabd
	@# run the gridlabd validation test
	docker run -it $(GRIDLABD_IMAGE) gridlabd -W /usr/local/src/gridlabd --validate
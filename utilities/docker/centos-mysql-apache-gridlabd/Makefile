# configuration variables
include Makefile.conf

# automatically constructed variables
GRIDLABD_IMAGE=$(GRIDLABD_IMG):$(GRIDLABD_TAG)
MYSQLD_IMAGE=$(MYSQLD_IMG):$(MYSQLD_TAG)
HTTPD_IMAGE=$(HTTPD_IMG):$(HTTPD_TAG)

help:
	@echo "Valid targets:"
	@echo "  install         pulls and installs the docker images"
	@echo "  clean           cleans up all docker images"
	@echo "  clean-mysqld    cleans up mysqld docker images"
	@echo "  clean-httpd     cleans up httpd docker images"
	@echo "  clean-gridlabd  cleans up gridlabd docker images"
	@echo "  gridlabd        pulls and starts the gridlabd image from $(GRIDLABD_IMAGE)"
	@echo "  mysqld          pulls and starts the mysqld image from $(MYSQLD_IMAGE)"
	@echo "  httpd           pulls and starts the httpd image from $(HTTPD_IMAGE)"
	@echo "  validate        run a validation test of the current install"
	@echo "  start           starts the gridlabd, mysqld, and httpd services"
	@echo "  stop            stops the gridlabd, mysqld, and httpd services"
	@echo "  build           creates the gridlabd image on local disk from $(GRIDLABD_IMAGE)"
	@echo "  push            pushes the gridlabd image to the docker repository"
	@echo "  help            displays this list (default)"

install: gridlabd mysqld httpd

start: 
	# TODO start the services

stop: 
	# TODO stop the services

restart: stop start #

clean: clean-gridlabd clean-mysqld clean-httpd

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

clean-gridlabd:
	@# clean up running image
	docker kill gridlabd 2>/dev/null || true
	docker rm gridlabd 2>/dev/null || true
	docker rmi -f $(GRIDLABD_IMAGE) gridlabd 2>/dev/null || true
	rm -f gridlabd.img dockerfile.tmp
	docker system prune -f
	
gridlabd.img: DockerFile system.sh gridlabd.sh Makefile.conf
	@# build a new image from scripts
	@[ ! -f dockerfile.tmp ] || ( echo "ERROR: a build is already running (dockerfile.tmp exists--try 'make clean')."; fail)
	./make_dockerfile DockerFile Makefile.conf dockerfile.tmp
	docker build -f dockerfile.tmp -t $(GRIDLABD_IMAGE) . && docker save $(GRIDLABD_IMAGE) -o gridlabd.img ; rm -f dockerfile.tmp
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

mysqld: Makefile.conf
	@# pull image
	@docker kill mysqld mysql56 1>/dev/null 2>&1 || true
	@docker rm -f mysqld mysql56 1>/dev/null 2>&1 || true
	@docker rmi -f $(MYSQLD_IMAGE) 1>/dev/null 2>&1 || true
	@docker system prune -f 1>/dev/null 2>&1 || true
	docker pull $(MYSQLD_IMAGE)
	@# start service
	docker run --name=mysql56 -p 3306:3306 -e MYSQL_ROOT_PASSWORD=gridlabd -v $$(pwd)/mysqld:/var/lib/mysql --rm -dit $(MYSQLD_IMAGE)
	@echo "Waiting for mysql to start..."
	@while [ "$$(docker inspect -f '{{.State.Health.Status}}' mysql56)" == 'starting' ]; do sleep 1; done
	@docker logs mysql56
	@test $$(docker inspect -f '{{.State.Health.Status}}' mysql56) == 'running'
	@echo "mysqld started ok."
	@# set passwords
	@#docker exec mysql56 mysqladmin --user=root --port=3306 --password="$$(docker logs mysqld 2>&1 | grep GENERATED | cut -f5 -d' ')" password gridlabd
	@# enable access by gridlabd service
	@#docker exec -i mysql56 mysql -uroot -pgridlabd < mysql-setup.sql

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
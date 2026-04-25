# Local helpers for Postgres setup/teardown and running the indexer.
# Override with env vars: DB_HOST, DB_PORT, DB_NAME, DB_USER, DB_PASS, DB_SUPER, PGPASSWORD_SUPER

DB_HOST ?= localhost
DB_PORT ?= 5432
DB_NAME ?= canton_db
DB_USER ?= canton_user
DB_PASS ?= 1234

# Use your current OS user by default
DB_SUPER ?= $(USER)
DB_SUPER_DB ?= postgres
PGPASSWORD_SUPER ?=

PSQL = PGPASSWORD=$(PGPASSWORD_SUPER) psql -h $(DB_HOST) -p $(DB_PORT) -U $(DB_SUPER) -d $(DB_SUPER_DB)
PSQL_USER = PGPASSWORD=$(DB_PASS) psql -h $(DB_HOST) -p $(DB_PORT) -U $(DB_USER) -d $(DB_NAME)

BUILD_DIR ?= build
CMAKE ?= cmake
CONFIG ?= config.local.toml

.PHONY: db-setup db-check db-clean configure build run help

# Create role + database for the local-node indexer.
db-setup:
	@echo "[db] ensuring role $(DB_USER) exists"
	@$(PSQL) -v ON_ERROR_STOP=1 -c ' \
	DO $$$$ \
	BEGIN \
		IF NOT EXISTS ( \
			SELECT FROM pg_catalog.pg_roles WHERE rolname = '\''$(DB_USER)'\'' \
		) THEN \
			CREATE ROLE $(DB_USER) LOGIN PASSWORD '\''$(DB_PASS)'\''; \
		END IF; \
	END \
	$$$$;'

	@echo "[db] ensuring database $(DB_NAME) exists"
	@$(PSQL) -v ON_ERROR_STOP=1 -tc \
	"SELECT 1 FROM pg_database WHERE datname='$(DB_NAME)';" | grep -q 1 || \
	$(PSQL) -v ON_ERROR_STOP=1 -c \
	"CREATE DATABASE $(DB_NAME) OWNER $(DB_USER);"

	@echo "[db] granting privileges"
	@$(PSQL) -v ON_ERROR_STOP=1 -d $(DB_NAME) -c \
	"GRANT ALL PRIVILEGES ON DATABASE $(DB_NAME) TO $(DB_USER);"

	@echo "[db] ready: postgres://$(DB_USER):******@$(DB_HOST):$(DB_PORT)/$(DB_NAME)"

# Quick connectivity + perms check using app credentials.
db-check:
	@echo "[db] checking connectivity as $(DB_USER)"
	@$(PSQL_USER) -v ON_ERROR_STOP=1 -c \
	"SELECT 'ok' AS status, current_user, current_database();"
	@$(PSQL_USER) -v ON_ERROR_STOP=1 -c \
	"CREATE TABLE IF NOT EXISTS _makefile_check(id int primary key);"
	@$(PSQL_USER) -v ON_ERROR_STOP=1 -c \
	"DROP TABLE IF EXISTS _makefile_check;"
	@echo "[db] connection and basic DDL verified"

# Drop the local database and role (dangerous; use carefully).
db-clean:
	@echo "[db] dropping database $(DB_NAME)"
	@$(PSQL) -v ON_ERROR_STOP=1 -c \
	"SELECT pg_terminate_backend(pid) FROM pg_stat_activity WHERE datname='$(DB_NAME)';"
	@$(PSQL) -v ON_ERROR_STOP=1 -c \
	"DROP DATABASE IF EXISTS $(DB_NAME);"
	@echo "[db] dropping role $(DB_USER)"
	@$(PSQL) -v ON_ERROR_STOP=1 -c \
	"DROP ROLE IF EXISTS $(DB_USER);"

# Configure build directory once
configure:
	@$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release
	
configure_debug:
	@$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug
	

# Build the project
build: configure
	@$(CMAKE) --build $(BUILD_DIR)
	
build_debug: configure_debug
	@$(CMAKE) --build $(BUILD_DIR)

# Build then start the indexer
run: build
	@echo "[run] starting indexer with $(CONFIG)"
	@set -a; source ./.env; set +a; LOCAL_NODE_CONFIG=$(CONFIG) $(BUILD_DIR)/bin/exe
	
run_debug: build_debug
	@echo "[run] starting indexer with $(CONFIG)"
	@set -a; source ./.env; set +a; LOCAL_NODE_CONFIG=$(CONFIG) $(BUILD_DIR)/bin/exe

run_debug_mainnet: build_debug
	@echo "[run] MAINNET starting indexer with $(CONFIG)"
	@set -a; source ./.env.prod; set +a; LOCAL_NODE_CONFIG=$(CONFIG) $(BUILD_DIR)/bin/exe

run_mainnet: build
	@echo "[run] MAINNET starting indexer with $(CONFIG)"
	@set -a; source ./.env.prod; set +a; LOCAL_NODE_CONFIG=$(CONFIG) $(BUILD_DIR)/bin/exe

run_debug_mainnet_valgrind: build_debug
	@echo "[run] MAINNET starting indexer with $(CONFIG)"
	@set -a; source ./.env.prod; set +a; LOCAL_NODE_CONFIG=$(CONFIG) valgrind --leak-check=full --show-leak-kinds=all --log-file="valgrind_report.txt" $(BUILD_DIR)/bin/exe
	
run_debug_devnet: build_debug
	@echo "[run] DEVNET starting indexer with $(CONFIG)"
	@set -a; source ./.env.dev; set +a; LOCAL_NODE_CONFIG=$(CONFIG) $(BUILD_DIR)/bin/exe

run_devnet: build
	@echo "[run] DEVNET starting indexer with $(CONFIG)"
	@set -a; source ./.env.dev; set +a; LOCAL_NODE_CONFIG=$(CONFIG) $(BUILD_DIR)/bin/exe

help:
	@echo "Available targets:"
	@echo "  db-setup    Create role/database for local use"
	@echo "  db-check    Verify DB connectivity and basic DDL as app user"
	@echo "  db-clean    Drop DB and role (dangerous)"
	@echo "  configure   Generate build files in $(BUILD_DIR)"
	@echo "  build       Build the project"
	@echo "  run         Build then launch the indexer"

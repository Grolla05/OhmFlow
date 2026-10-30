# OhmFlow — build, testes e verificações
#   make            compila dist/ohmflow
#   make run        executa o programa
#   make test       testes unitários
#   make e2e        teste ponta a ponta (menu alimentado por tests/e2e/entrada.txt)
#   make asan       testes + e2e com ASan/UBSan (nao roda no MinGW; fora do CI)
#   make cppcheck   análise estática
#   make clean      remove build/ e dist/
#   make icone      regenera src/assets/logo/ohmflow.ico a partir do logo (Windows, Pillow)
#   make setup      gera dist/ohm_flow_setup.exe (Windows, Inno Setup 6) [VERSAO=x.y.z]
#   make teste-setup  instala o setup em silencio, roda o programa e desinstala

CC           = gcc
BUILD        = build
DIST         = dist
CFLAGS       = -std=c99 -Wall -Wextra -pedantic -g -Isrc -Isrc/ui -I$(BUILD)
EXTRA_CFLAGS =
# SQLite estático: o exe segue dependendo só de DLLs do Windows
LDLIBS       = -Wl,-Bstatic -lsqlite3 -Wl,-Bdynamic -lm
VERSAO      ?= 1.0.0

# "1.2.3-ci" -> "1,2,3,0" (FILEVERSION numérico do recurso VERSIONINFO)
virgula      = ,
VERSAO_NUM   = $(subst .,$(virgula),$(firstword $(subst -, ,$(VERSAO)))),0

ifeq ($(OS),Windows_NT)
  EXE     = .exe
  WINDRES = windres
  ICO     = src/assets/logo/ohmflow.ico
  RES_OBJ = $(BUILD)/ohmflow_res.o
  # Subsistema console (CUI): cmd/PowerShell esperam o programa terminar (issue #11)
  CHECA_SUBSISTEMA = objdump -p $(BIN) | grep -q '(Windows CUI)' \
                     || { echo "E2E FALHOU: $(BIN) nao usa o subsistema console (CUI)"; exit 1; }
else
  EXE     =
  WINDRES =
  ICO     =
  RES_OBJ =
  CHECA_SUBSISTEMA = true
endif

SRC_LIB   = src/lista.c src/pilha.c src/checkpoint.c src/formatacao.c src/resistor.c src/log.c src/persistencia.c src/ui/ui.c src/ui/telas.c
SRC_MAIN  = src/main.c
SCHEMA    = database/database.sql
SCHEMA_H  = $(BUILD)/schema_sql.h
HEADERS   = $(wildcard src/*.h src/ui/*.h) $(SCHEMA_H)
BIN       = $(DIST)/ohmflow$(EXE)
TESTES    = test_lista test_pilha test_checkpoint test_formatacao test_log test_ui test_telas test_novo test_resistor test_persistencia
TEST_BINS = $(addprefix $(BUILD)/,$(addsuffix $(EXE),$(TESTES)))

E2E_ENTRADA  = tests/e2e/entrada.txt
E2E_ESPERADO = tests/e2e/esperado.txt
E2E_SAIDA    = $(BUILD)/e2e_saida.txt
E2E_LOGS     = $(BUILD)/e2e_logs
E2E_DB       = $(BUILD)/e2e.db
E2E_ENTRADA_2  = tests/e2e/entrada_persistencia.txt
E2E_ESPERADO_2 = tests/e2e/esperado_persistencia.txt
E2E_SAIDA_2    = $(BUILD)/e2e_saida_persistencia.txt

ASAN_FLAGS = -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer

SETUP = $(DIST)/ohm_flow_setup.exe
# exe do instalador: igual ao de dev, mas grava logs e banco em %APPDATA%\OhmFlow
BIN_SETUP = $(DIST)/instalador/ohmflow$(EXE)

.PHONY: all run test e2e asan cppcheck clean icone setup teste-setup

all: $(BIN)

$(BUILD):
	mkdir -p $(BUILD)

# database.sql vira uma string C por linha; persistencia.c inclui o resultado
$(SCHEMA_H): $(SCHEMA) | $(BUILD)
	sed -e 's/\r$$//' -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e 's/^/    "/' -e 's/$$/\\n"/' $< > $@

# O .ico é versionado: só é gerado (Python + Pillow) se estiver faltando, para
# um checkout limpo não depender da data de logo.png. Para regenerar após
# trocar o logo: make icone
GERA_ICO = python -c "from PIL import Image; img=Image.open('src/assets/logo/logo.png').convert('RGBA'); img.save('$(ICO)', format='ICO', sizes=[(16,16),(32,32),(48,48),(256,256)])"

$(ICO): | src/assets/logo/logo.png
	$(GERA_ICO)

icone: src/assets/logo/logo.png
	$(GERA_ICO)

$(RES_OBJ): src/assets/ohmflow.rc $(ICO) | $(BUILD)
	$(WINDRES) -DVERSAO=$(VERSAO) -DVERSAO_NUM=$(VERSAO_NUM) $< -o $@

$(BIN): $(SRC_MAIN) $(SRC_LIB) $(RES_OBJ) $(HEADERS)
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ $(SRC_MAIN) $(SRC_LIB) $(RES_OBJ) $(LDLIBS)

$(BIN_SETUP): $(SRC_MAIN) $(SRC_LIB) $(RES_OBJ) $(HEADERS)
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -DOHM_LOGS_APPDATA -DOHM_DADOS_APPDATA -o $@ $(SRC_MAIN) $(SRC_LIB) $(RES_OBJ) $(LDLIBS)

$(BUILD)/test_%$(EXE): tests/test_%.c tests/test_framework.h $(SRC_LIB) $(HEADERS) | $(BUILD)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -Itests -o $@ $< $(SRC_LIB) $(LDLIBS)

run: $(BIN)
	./$(BIN)

test: $(TEST_BINS)
	@falhou=0; for t in $(TEST_BINS); do ./$$t || falhou=1; done; exit $$falhou

# Roda o binário real com a entrada roteirizada e confere, EM ORDEM, cada linha
# de esperado.txt contra a saída (linhas vazias de esperado.txt são ignoradas).
# Uma segunda execução confere que o estoque voltou do banco ($(E2E_DB)).
# Os logs vão para $(E2E_LOGS) e são conferidos no fim (cabeçalho, ação, rodapé).
E2E_CONFERE = awk 'NR==FNR { sub(/\r$$/, ""); if (length($$0)) esp[++n] = $$0; next } \
	     { sub(/\r$$/, ""); if (i < n && index($$0, esp[i+1])) i++ } \
	     END { if (i < n) { print "E2E FALHOU: nao encontrado (em ordem): " esp[i+1]; \
	                        print "Saida completa em $(2)"; exit 1 } \
	           print "E2E OK (" n " verificacoes)" }' $(1) $(2)

e2e: $(BIN) | $(BUILD)
	@$(CHECA_SUBSISTEMA)
	@rm -rf $(E2E_LOGS) $(E2E_DB)
	@NO_COLOR=1 OHMFLOW_LOGS_DIR=$(E2E_LOGS) OHMFLOW_DB=$(E2E_DB) ./$(BIN) < $(E2E_ENTRADA) > $(E2E_SAIDA) 2>&1 || { echo "E2E FALHOU: programa saiu com erro"; exit 1; }
	@$(call E2E_CONFERE,$(E2E_ESPERADO),$(E2E_SAIDA))
	@# segunda sessão, sem cadastrar nada: o que a primeira salvou tem de voltar
	@NO_COLOR=1 OHMFLOW_LOGS_DIR=$(E2E_LOGS) OHMFLOW_DB=$(E2E_DB) ./$(BIN) < $(E2E_ENTRADA_2) > $(E2E_SAIDA_2) 2>&1 || { echo "E2E FALHOU: segunda sessao saiu com erro"; exit 1; }
	@$(call E2E_CONFERE,$(E2E_ESPERADO_2),$(E2E_SAIDA_2))
	@log="$$(ls $(E2E_LOGS)/ohmflow_*.log 2>/dev/null | head -1)"; \
	[ -n "$$log" ] || { echo "E2E FALHOU: log nao criado em $(E2E_LOGS)"; exit 1; }; \
	for trecho in 'sessão iniciada em' \
	              '| INFO  | NOVO          | 4,7 kΩ     | qtd 10               | OK' \
	              '| AVISO | RETIRAR       | 4,7 kΩ     | -100 (tem 7)         | SEM ESTOQUE' \
	              '| ERRO  | NOVO          | "abc"' \
	              '| INFO  | NOVO          | -          | 3 ok · 1 dup · 1 inv | LOTE CONCLUÍDO' \
	              '| INFO  | DESFAZER      | 3 tipos    | NOVO lote            | OK' \
	              '| INFO  | REFAZER       | -          | REINICIALIZAR 3 tipos | OK' \
	              '| INFO  | CHECKPOINT    | -          | ponto 11 → 9         | OK' \
	              '| INFO  | BANCO         | -          | carga                | OK' \
	              '| INFO  | BANCO         | -          | salvamento           | OK' \
	              'sessão encerrada em'; do \
	  grep -qF -- "$$trecho" "$$log" || { echo "E2E FALHOU: log sem: $$trecho (veja $$log)"; exit 1; }; \
	done; \
	echo "E2E LOG OK ($$log)"

asan:
	$(MAKE) BUILD=$(BUILD)/asan DIST=$(BUILD)/asan EXTRA_CFLAGS="$(EXTRA_CFLAGS) $(ASAN_FLAGS)" test e2e

# Instalador do usuário final. Procura o compilador do Inno Setup no PATH e nas
# pastas padrão (Arquivos de Programas ou %LOCALAPPDATA%\Programs, onde o winget
# instala por usuário). MSYS2_ARG_CONV_EXCL impede o MSYS de converter /Q e
# /DVersao=... em caminhos.
setup: $(BIN_SETUP)
	@iscc="$$(command -v iscc 2>/dev/null)"; \
	for c in "/c/Program Files (x86)/Inno Setup 6/ISCC.exe" \
	         "$$(cygpath -u "$$LOCALAPPDATA" 2>/dev/null)/Programs/Inno Setup 6/ISCC.exe"; do \
	  if [ -z "$$iscc" ] && [ -f "$$c" ]; then iscc="$$c"; fi; \
	done; \
	[ -n "$$iscc" ] || { echo "SETUP FALHOU: ISCC.exe (Inno Setup 6) nao encontrado. Instale com: winget install JRSoftware.InnoSetup"; exit 1; }; \
	MSYS2_ARG_CONV_EXCL='*' "$$iscc" /Q /DVersao=$(VERSAO) /DExeOrigem=$(BIN_SETUP) installer/ohmflow.iss
	@echo "Instalador gerado: $(SETUP)"

teste-setup: setup
	powershell -NoProfile -File tests/setup/test_setup.ps1

cppcheck: $(SCHEMA_H)
	cppcheck --enable=warning,style,performance --error-exitcode=1 --inline-suppr --quiet -Isrc -Isrc/ui -I$(BUILD) src/

clean:
	rm -rf $(BUILD) $(DIST)

CC = gcc
CFLAGS = -Wall `pkg-config --cflags gtk+-3.0`
LIBS = `pkg-config --libs gtk+-3.0`

# Dossier de build
BUILD_DIR = build

# Fichiers sources de l'interface
SRC = src/main.c \
      gui/gui.c \
      gui/menu.c \
      gui/liste.c \
      gui/tabs.c \
      gui/stats.c \
      src/run_algorithm.c \
      gui/gui_builder.c

OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRC))

# --- DÉTECTION AUTOMATIQUE DES POLICIES ---
# Trouve tous les .c dans policies/ et génère les noms d'exécutables dans build/
POLICY_SRC = $(filter-out %_utils.c policies/_.c, $(wildcard policies/*.c))
POLICIES = $(patsubst policies/%.c,$(BUILD_DIR)/%,$(POLICY_SRC))

# Cible principale : tout compiler
all: $(BUILD_DIR) ordonnanceur $(POLICIES)

# Nouvelle cible : compiler et exécuter automatiquement
run: all
	@echo "=========================================="
	@echo "  Lancement de l'ordonnanceur..."
	@echo "=========================================="
	@GDK_SYNCHRONIZE=0 ./ordonnanceur 2>/dev/null || ./ordonnanceur

# Créer le dossier build/ et ses sous-dossiers
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)/src
	@mkdir -p $(BUILD_DIR)/gui

# Compilation de l'interface graphique
ordonnanceur: $(OBJ)
	@echo "🔨 Compilation de l'interface graphique..."
	@$(CC) $(OBJ) -o ordonnanceur $(LIBS)
	@echo "✓ ordonnanceur compilé avec succès"

# --- RÈGLE GÉNÉRIQUE pour compiler les .c en .o dans build/ ---
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "  Compilation: $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# --- RÈGLE GÉNÉRIQUE pour compiler n'importe quelle policy dans build/ ---
$(BUILD_DIR)/%: policies/%.c
	@mkdir -p $(BUILD_DIR)
	@echo "📋 Compilation de $< → $@"
	@$(CC) -Wall $< -o $@

# Nettoyage
clean:
	@echo "🧹 Nettoyage des fichiers de build..."
	@rm -rf $(BUILD_DIR) ordonnanceur
	@echo "✓ Nettoyage terminé"

# Recompilation complète
rebuild: clean all

# Recompilation et exécution
rerun: clean run

# Afficher les policies détectées (utile pour debug)
show-policies:
	@echo "Fichiers sources détectés :"
	@echo "$(POLICY_SRC)"
	@echo ""
	@echo "Exécutables à générer :"
	@echo "$(POLICIES)"

.PHONY: all run clean rebuild rerun show-policies
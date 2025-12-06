cat > README.md << 'EOF'
# 🖥️ Ordonnanceur Multi-tâche

Simulateur d'algorithmes d'ordonnancement de processus avec interface graphique GTK3.

![GTK](https://img.shields.io/badge/GTK-3.0-green.svg)
![C](https://img.shields.io/badge/language-C-orange.svg)

## ✨ Fonctionnalités

- **Algorithmes d'ordonnancement** :
  - 🔹 FIFO (First In, First Out)
  - 🔹 Round Robin (avec quantum configurable)
  - 🔹 SRT (Shortest Remaining Time - préemptif)

- **Interface graphique moderne** :
  - Thème sombre élégant
  - Diagramme de Gantt animé avec Cairo
  - Visualisation temps réel CPU/E-S
  - Statistiques détaillées

- **Statistiques** :
  - Temps de rotation moyen
  - Temps d'attente moyen
  - Support parallèle CPU/E-S
## 🚀 Installation

### Prérequis
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install build-essential libgtk-3-dev

# Fedora/RHEL
sudo dnf install gcc gtk3-devel

# Arch Linux
sudo pacman -S base-devel gtk3
```

### Compilation
```bash
git clone https://github.com/VOTRE_USERNAME/OS-PROJECT.git
cd OS-PROJECT
make
```

### Exécution
```bash
./ordonnanceur
```

## 📖 Utilisation

### 1. Interface Graphique

1. Lancez l'application : `./ordonnanceur`
2. Sélectionnez un algorithme dans le menu déroulant
3. Pour Round Robin, entrez un quantum (ex: 2, 5, 10)
4. Cliquez sur **"▶ Démarrer"**
5. Visualisez le diagramme de Gantt et les statistiques

### 2. Format du fichier d'entrée

Le fichier `config/input.txt` contient la description des processus :
```
# Format: Nom Arrivée Burst1 Burst2 Burst3 ... Priorité
# Les bursts alternent: CPU, E/S, CPU, E/S...

P1 0 3 3 4 1
P2 1 6 2
P3 2 1 12 2 3
```

**Explication** :
- `P1` : Nom du processus
- `0` : Temps d'arrivée
- `3 3 4` : Bursts (3 CPU, 3 E/S, 4 CPU)
- `1` : Priorité (pour algorithmes futurs)

### 3. Exemples

#### FIFO
```bash
./ordonnanceur
# Sélectionner "fifo" → Démarrer
```

#### Round Robin (quantum=2)
```bash
./ordonnanceur
# Sélectionner "rr" → Entrer "2" → Démarrer
```

## 🏗️ Architecture
```
OS-PROJECT/
├── gui/
│   ├── gui.c              # Contrôleur (logique + callbacks)
│   ├── gui_builder.c      # Vue (widgets + CSS)
│   ├── menu.c             # Point d'entrée GTK
│   ├── liste.c            # Détection des algorithmes
│   └── tabs.c             # Diagramme de Gantt (Cairo)
├── policies/
│   ├── fifo.c             # Algorithme FIFO
│   ├── rr.c               # Algorithme Round Robin
│   └── srt.c              # Algorithme SRT
├── src/
│   ├── main.c             # Point d'entrée
│   └── run_algorithm.c    # Exécution des algorithmes
├── config/
│   └── input.txt          # Fichier de test
├── makefile               # Compilation automatique
└── README.md
```

### Pattern MVC

- **Modèle** : `policies/*.c` (algorithmes)
- **Vue** : `gui_builder.c` (interface)
- **Contrôleur** : `gui.c` (logique)

## 🛠️ Développement

### Ajouter un nouvel algorithme

1. Créez `policies/mon_algo.c`
2. Implémentez la fonction `main()` qui lit depuis `stdin`
3. Recompilez : `make`
4. L'algorithme apparaîtra automatiquement dans l'interface !

### Makefile dynamique

Le Makefile détecte automatiquement tous les fichiers `.c` dans `policies/` :
```makefile
POLICY_SRC = $(wildcard policies/*.c)
POLICIES = $(POLICY_SRC:.c=)
```

## 📊 Algorithmes Implémentés

### FIFO (First In, First Out)
- Non préemptif
- Ordre d'arrivée strict
- Simple mais peut causer de la famine

### Round Robin
- Préemptif avec quantum
- Équitable entre processus
- Bon pour systèmes interactifs

### SRT (Shortest Remaining Time)
- Préemptif
- Optimal pour temps d'attente moyen
- Favorise les courts jobs

## 🧪 Tests
```bash
# Tester FIFO
echo "P1 0 5 1" > config/input.txt
./policies/fifo < config/input.txt

# Tester RR avec quantum=2
echo -e "quantum 2\nP1 0 8 1" | ./policies/rr

# Tester SRT
echo "P1 0 8 1\nP2 1 4 2" > config/input.txt
./policies/srt < config/input.txt

# Guide d'Installation Détaillé pour Windows

Ce guide vous expliquera comment compiler et lancer le projet **SED-Lab** sur un environnement Windows. La méthode recommandée utilise le compilateur MSVC (inclus avec Visual Studio) et le gestionnaire de paquets `vcpkg` pour les dépendances.

---

## 1. Prérequis : Installer les Outils

Avant de commencer, vous devez installer les outils suivants.

### A. Git (Contrôle de version)

Si vous ne l'avez pas, téléchargez et installez Git pour Windows.
- **Lien :** [git-scm.com/download/win](https://git-scm.com/download/win)

### B. Visual Studio 2022 Community

Visual Studio fournit le compilateur C++ (MSVC) et les outils de build nécessaires.

1.  **Téléchargez Visual Studio Community :**
    Rendez-vous sur la [page de téléchargement de Visual Studio](https://visualstudio.microsoft.com/downloads/) et téléchargez **"Visual Studio Community 2022"**. C'est gratuit pour les développeurs individuels.

2.  **Installez la charge de travail C++ :**
    Pendant l'installation, sélectionnez la charge de travail **"Développement Desktop en C++"**. Assurez-vous que les composants suivants sont cochés sur le panneau de droite :
    *   **MSVC v143 - VS 2022 C++ x64/x86 build tools** (ou plus récent)
    *   **Windows 11 SDK** (ou la version pour votre OS)

---

## 2. Installation des Dépendances avec vcpkg

`vcpkg` est un gestionnaire de paquets qui simplifie l'installation des bibliothèques C++ comme `raylib`.

1.  **Ouvrez une invite de commandes (`cmd` ou `PowerShell`).**

2.  **Clonez le dépôt de vcpkg :**
    Nous recommandons un chemin simple, par exemple `C:\dev\vcpkg`.
    ```bash
    git clone https://github.com/Microsoft/vcpkg.git C:\dev\vcpkg
    ```

3.  **Installez vcpkg :**
    ```bash
    cd C:\dev\vcpkg
    .\bootstrap-vcpkg.bat
    ```

4.  **Installez raylib pour une architecture 64-bit :**
    Cette commande va télécharger, compiler et installer `raylib`. Cela peut prendre plusieurs minutes.
    ```bash
    .\vcpkg.exe install raylib:x64-windows
    ```
    *Note : L'ancienne commande `integrate install` n'est plus recommandée. Nous spécifierons le chemin vers vcpkg directement lors de la compilation.*

---

## 3. Compilation et Lancement du Projet

Maintenant que tous les outils sont prêts, nous pouvons compiler le simulateur.

1.  **Clonez le projet SED-Lab :**
    Si ce n'est pas déjà fait, clonez le dépôt du projet dans un dossier de votre choix (par exemple, `C:\dev\SED`).
    ```bash
    git clone https://github.com/votre-utilisateur/Simulateur-d-emergence-D-eterministe.git C:\dev\SED
    cd C:\dev\SED
    ```
    *(Remplacez l'URL par celle de votre dépôt si nécessaire.)*

2.  **Ouvrez l'Invite de Commandes Développeur de Visual Studio :**
    C'est l'étape la plus importante pour éviter les erreurs de compilateur. Cherchez dans le menu Démarrer :
    `x64 Native Tools Command Prompt for VS 2022`
    Lancez-le. Une nouvelle fenêtre de terminal s'ouvrira, préconfigurée pour utiliser les outils de Visual Studio.

3.  **Naviguez jusqu'au dossier du projet dans ce nouveau terminal :**
    ```cmd
    cd C:\dev\SED
    ```

4.  **Créez un dossier de build et compilez :**
    Exécutez les commandes suivantes une par une.
    ```cmd
    mkdir build
    cd build
    ```

5.  **Configurez le projet avec CMake (L'étape clé) :**
    Cette commande indique à CMake où trouver les bibliothèques de `vcpkg`.
    ```cmd
    cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake
    ```
    *(Adaptez le chemin si vous avez installé vcpkg ailleurs.)*

6.  **Compilez le projet :**
    ```cmd
    cmake --build .
    ```
    L'exécutable `sed_lab.exe` sera créé dans le dossier `build\Debug`.

7.  **Lancez l'Application :**
    ```cmd
    .\Debug\sed_lab.exe
    ```

Vous devriez maintenant voir la fenêtre de l'application SED-Lab s'ouvrir.
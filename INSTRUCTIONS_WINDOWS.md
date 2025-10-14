# Guide d'Installation pour Windows

Ce guide vous expliquera comment compiler et lancer le projet **SED-Lab** sur un environnement Windows. La méthode recommandée utilise le compilateur MSVC (inclus avec Visual Studio), le gestionnaire de paquets `vcpkg` pour les dépendances, et CMake pour la compilation.

---

## 1. Prérequis Essentiels

Avant de commencer, vous devez installer les outils suivants :

### A. Visual Studio 2022

Visual Studio fournit le compilateur C++ (MSVC), CMake, et Git, qui sont tous nécessaires pour ce projet.

1.  **Téléchargez Visual Studio Community :**
    Rendez-vous sur la [page de téléchargement de Visual Studio](https://visualstudio.microsoft.com/downloads/) et téléchargez "Visual Studio Community 2022". C'est gratuit pour les développeurs individuels et les projets open-source.

2.  **Installez la charge de travail C++ :**
    Lors de l'installation, sélectionnez la charge de travail **"Développement Desktop en C++"**. Assurez-vous que les composants suivants sont cochés sur le panneau de droite :
    *   **MSVC v143 - VS 2022 C++ x64/x86 build tools** (ou une version plus récente)
    *   **Windows 11 SDK** (ou une version pour votre OS)
    *   **CMake**
    *   **Git for Windows**

### B. vcpkg (Gestionnaire de Dépendances)

`vcpkg` est un gestionnaire de paquets de Microsoft qui simplifie grandement l'installation des bibliothèques C++ comme `raylib`.

1.  **Ouvrez une invite de commandes :**
    Ouvrez `PowerShell` ou `CMD`.

2.  **Clonez le dépôt de vcpkg :**
    Nous vous recommandons de le cloner dans un chemin simple (par exemple, `C:\vcpkg`).
    ```bash
    git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
    ```

3.  **Installez vcpkg :**
    Exécutez le script de bootstrap.
    ```bash
    cd C:\vcpkg
    .\bootstrap-vcpkg.bat
    ```

4.  **Intégrez vcpkg avec votre système (très important) :**
    Cette commande permet à CMake de trouver automatiquement les bibliothèques installées par `vcpkg`.
    ```bash
    .\vcpkg.exe integrate install
    ```

---

## 2. Installation des Dépendances

Maintenant que `vcpkg` est prêt, utilisez-le pour installer `raylib`.

1.  **Ouvrez une invite de commandes (`PowerShell` ou `CMD`).**

2.  **Installez raylib pour une architecture 64-bit :**
    ```bash
    C:\vcpkg\vcpkg.exe install raylib:x64-windows
    ```
    L'installation peut prendre quelques minutes, car `vcpkg` va télécharger, compiler et installer la bibliothèque pour vous.

---

## 3. Compilation et Lancement du Projet

Le projet utilise CMake, ce qui le rend compatible avec de nombreux environnements de développement.

### A. Cloner le Projet SED-Lab

Si ce n'est pas déjà fait, clonez le dépôt du projet :
```bash
git clone <URL_DU_PROJET_SED_LAB>
cd <NOM_DU_DOSSIER_DU_PROJET>
```

### B. Compiler avec CMake

1.  **Créez un dossier de build :**
    C'est une bonne pratique de séparer les fichiers de compilation des fichiers sources.
    ```bash
    mkdir build
    cd build
    ```

2.  **Générez les fichiers de projet avec CMake :**
    CMake va détecter automatiquement les bibliothèques installées via `vcpkg`.
    ```bash
    cmake ..
    ```
    Si tout se passe bien, CMake générera une solution Visual Studio (`.sln`) dans le dossier `build`.

3.  **Compilez le projet :**
    Vous pouvez soit ouvrir la solution `.sln` avec Visual Studio et compiler depuis l'interface, soit utiliser CMake directement depuis la ligne de commande :
    ```bash
    cmake --build .
    ```
    Un exécutable `sed_lab.exe` sera créé dans le dossier `build\Debug`.

### C. Lancer l'Application

Une fois la compilation terminée, vous pouvez lancer l'exécutable :
```bash
.\Debug\sed_lab.exe
```

Vous devriez maintenant voir la fenêtre de l'application SED-Lab s'ouvrir.

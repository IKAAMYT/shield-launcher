# AlterCOD Launcher — prêt à l'emploi

Repo Shield/BO4 entièrement rebrandé AlterCOD (couleurs or/noir, logo BO4 doré,
nom, image de fond) + un système de build automatique sans rien installer.

## CE QUI EST DÉJÀ FAIT
- Thème or #f2c411 sur noir #0a0a08 (boutons bordure dorée, slider, progressbar…)
- Titre fenêtre : "AlterCOD Launcher" / bouton "AlterCOD Docs" / updater "AlterCOD updater"
- Métadonnées de l'exe rebrandées (resources.rc)
- Icône : logo BO4 (4 barres bouclier) en doré — source/launcher/icon.ico + launcher_shortcut/icon.ico
- Image de fond : launcher.png AlterCOD (hexagones dorés + "ALTERCOD / Launcher BO4")
- Coquille corrigée dans deps/premake/qt6.lua (QtNNetwork -> QtNetwork)
- Workflow GitHub Actions : .github/workflows/build.yml

============================================================
## OPTION A — COMPILER SANS RIEN INSTALLER (recommandé)
============================================================
1. Crée un repo GitHub (ou utilise le tien) et pousse ces fichiers dedans.
2. Va dans l'onglet "Actions" du repo sur GitHub.
3. Le build se lance tout seul au push. Sinon : clique sur "Build AlterCOD
   Launcher" puis "Run workflow".
4. Quand c'est vert (quelques minutes), ouvre le run et télécharge l'artefact
   "AlterCOD-Launcher" en bas de la page : il contient l'exe + les DLL Qt.

Rien à installer sur ton PC. GitHub compile pour toi sur une machine Windows.

============================================================
## OPTION B — COMPILER EN LOCAL (Windows)
============================================================
Prérequis (une seule fois) :
- Visual Studio 2022 Community + charge de travail "Développement Desktop en C++"
- Qt 6.7.x (composant MSVC 2022 64-bit) via le Qt Online Installer

Étapes :
1. Copie ton Qt dans le projet (premake l'attend ici) :
     deps/qt6/include/  <- depuis  C:\Qt\6.7.x\msvc2022_64\include
     deps/qt6/lib/      <- depuis  C:\Qt\6.7.x\msvc2022_64\lib
     deps/qt6/bin/      <- depuis  C:\Qt\6.7.x\msvc2022_64\bin
2. Double-clic sur generate.bat
3. Ouvre build\Shield_launcher.sln dans Visual Studio
4. Config : Release / x64, puis F7 (Générer la solution)
5. L'exe sort dans build\bin\... avec les DLL Qt à côté

============================================================
## NOTE IMPORTANTE — deps/ et project-bo4/
============================================================
Ce dossier ne contient PAS le dossier deps/qt6/ (Qt fait plusieurs Go).
- Option A : le workflow le télécharge tout seul.
- Option B : tu le copies toi-même (étape 1 ci-dessus).
Les dossiers project-bo4/ (fichiers du jeu) et les noms techniques (.json, DLL)
n'ont pas été touchés : les renommer casserait le launcher.

Le nom de l'exe reste Shield_Launcher.exe (référencé par le raccourci et
ta release GitHub). Le rebranding visible (titre, icône, couleurs) est complet.

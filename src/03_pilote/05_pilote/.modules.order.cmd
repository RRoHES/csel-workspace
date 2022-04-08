cmd_/workspace/src/03_pilote/05_pilote/modules.order := {   echo /workspace/src/03_pilote/05_pilote/driver_05.ko; :; } | awk '!x[$$0]++' - > /workspace/src/03_pilote/05_pilote/modules.order

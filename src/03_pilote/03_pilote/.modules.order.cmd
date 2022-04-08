cmd_/workspace/src/03_pilote/03_pilote/modules.order := {   echo /workspace/src/03_pilote/03_pilote/multi_minor.ko; :; } | awk '!x[$$0]++' - > /workspace/src/03_pilote/03_pilote/modules.order

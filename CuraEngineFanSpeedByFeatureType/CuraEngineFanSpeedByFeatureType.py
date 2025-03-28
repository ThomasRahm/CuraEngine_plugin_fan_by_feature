import os
import platform
import stat
import sys
from pathlib import Path

from UM.Logger import Logger
from UM.Settings.ContainerRegistry import ContainerRegistry
from UM.Settings.DefinitionContainer import DefinitionContainer
from UM.i18n import i18nCatalog
from cura.BackendPlugin import BackendPlugin

catalog = i18nCatalog("cura")


class CuraEngineFanSpeedByFeatureType(BackendPlugin):
    def __init__(self):
        super().__init__()
        self.definition_file_paths = [Path(__file__).parent.joinpath("fan_settings.def.json").as_posix()]
        if not self.isDebug():
            if not self.binaryPath().exists():
                Logger.error(f"Could not find CuraEngineFanSpeedByFeatureType binary at {self.binaryPath().as_posix()}")
            if platform.system() != "Windows" and self.binaryPath().exists():
                st = os.stat(self.binaryPath())
                os.chmod(self.binaryPath(), st.st_mode | stat.S_IEXEC)

            self._plugin_command = [self.binaryPath().as_posix()]

        self._supported_slots = [103]  # ModifyPostprocess SlotID
        ContainerRegistry.getInstance().containerLoadComplete.connect(self._on_container_load_complete)

    def _on_container_load_complete(self, container_id) -> None:
       pass

    def getPort(self):
        return super().getPort() if not self.isDebug() else int(os.environ["CURAENGINE_FAN_BY_FEATURE_PORT"])

    def isDebug(self):
        return not hasattr(sys, "frozen") and os.environ.get("CURAENGINE_FAN_BY_FEATURE_PORT", None) is not None

    def start(self):
        if not self.isDebug():
            super().start()

    def binaryPath(self) -> Path:
        ext = ".exe" if platform.system() == "Windows" else ""

        machine = platform.machine()
        if machine == "AMD64":
            machine = "x86_64"
        return Path(__file__).parent.joinpath(machine, platform.system(), f"curaengine_plugin_fan_by_feature{ext}").resolve()

import configparser
import os

class Translator:
    def __init__(self):
        self.configs = {}

    def load_language(self, language_name):
        if language_name not in self.configs:
            config = configparser.ConfigParser()
            config.read(os.path.join(os.getcwd(), "lng", f"{language_name}.lang"))
            self.configs[language_name] = config

    def translate(self, language_name, string):
        if language_name == "en":
            return string
        elif language_name not in self.configs:
            self.load_language(language_name)
        config = self.configs[language_name]
        try:
            return config.get("Strings", string)
        except (configparser.NoOptionError, configparser.NoSectionError):
            if string:
                return string
            else:
                raise Exception("language engine error: This translation is corrupt!")
                return 0

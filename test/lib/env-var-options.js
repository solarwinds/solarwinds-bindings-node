
class EnvVarOptions {
  constructor (prefix, options = {}) {
    this.prefix = prefix;
    this.plength = prefix.length;
    this.vars = {};
    this.keyMap = options.keyMap;

    this.refresh();
  }

  refresh () {
    this.vars = {};
    Object.keys(process.env).forEach(k => {
      if (k.startsWith(this.prefix)) {
        this.vars[k.slice(this.plength)] = process.env[k];
      }
    })
  }

  getRaw () {
    return this.vars;
  }

  getConverted (keyMap, options = {}) {
    keyMap = keyMap || this.keyMap;
    const converter = options.converter || convert;
    if (!keyMap) {
      throw new TypeError('must supply keyMap option to get converted vars');
    }
    const converted = {};

    Object.keys(this.vars).forEach(k => {
      const keyMapEntry = keyMap[k];
      if (keyMapEntry) {
        const value = convert(this.vars[k], keyMapEntry.type);
        if (value !== undefined) {
          converted[keyMapEntry.name] = value;
        }
      }
    })
    return converted;
  }

}

function convert (string, type) {
  if (type === 's') {
    return string;
  }
  if (type === 'i') {
    const v = +string;
    return Number.isNaN(v) ? undefined : v;
  }
  if (type === 'b') {
    return ['true', '1', 'yes', 'TRUE', 'YES'].indexOf(string) >= 0
  }
  if (typeof type === 'object' && string.toLowerCase() in type) {
    return type[string.toLowerCase()];
  }
  return undefined;
}

module.exports = EnvVarOptions;

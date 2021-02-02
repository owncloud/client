module.exports = {
  /**
   * Returns an XPath string literal to use for searching values inside
   * element/attribute.
   *
   * This wraps them in single quotes, double quotes, or as a concat function,
   * as xpath expressions can contain single or double quotes but not both.
   * Also, xpath does not support escaping.
   *
   * {@link https://stackoverflow.com/q/1341847 Special Character in XPATH Query}
   * {@link https://stackoverflow.com/q/642125 Encoding XPath Expressions with both single and double quotes}
   *
   * @param {string} value
   * @returns {string}
   */
  buildXpathLiteral: function(value) {
    if (!value.includes("'")) {
      // if we don't have any single quotes, then wrap them with single quotes
      return "'" + value + "'"
    } else if (!value.includes('"')) {
      // if we don't have any double quotes, then wrap them with double quotes
      return '"' + value + '"'
    } else {
      // use concat to find the literal in the xpath if they contain both quotes
      return "concat('" + value.replace(/'/g, "',\"'\",'") + "')"
    }
  }
}

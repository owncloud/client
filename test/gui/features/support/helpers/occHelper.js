const httpHelper = require('../helpers/httpHelper')

/**
 * Run occ command using the testing API
 *
 * @param {Array} args
 */
exports.runOcc = function(args) {
  const params = new URLSearchParams()
  params.append('command', args.join(' '))
  const apiURL = 'apps/testing/api/v1/occ'
  return httpHelper
    .postOCS(apiURL, 'admin', params)
    .then(res => {
      httpHelper.checkStatus(res, 'Failed while executing occ command')
      return res.json()
    })
    .then(res => {
      httpHelper.checkOCSStatus(res, 'Failed while executing occ command')
      return res
    })
}

const { Given, After } = require('cucumber')
const httpHelper = require('./helpers/httpHelper')
const userSettings = require('./helpers/userSettings')
const codify = require('./helpers/codify')

function createDefaultUser(userId) {
  const password = userSettings.getPasswordForUser(userId)
  const displayname = userSettings.getDisplayNameOfDefaultUser(userId)
  const email = userSettings.getEmailAddressOfDefaultUser(userId)
  return createUser(userId, password, displayname, email)
}

function createUser(userId, password, displayName = false, email = false) {
  const body = new URLSearchParams()
  body.append('userid', userId)
  body.append('password', password)
  body.append('displayname', displayName)
  const promiseList = []

  userSettings.addUserToCreatedUsersList(userId, password, displayName, email)
  const url = 'cloud/users'
  return httpHelper
    .postOCS(url, 'admin', body)
    .then(res => httpHelper.checkStatus(res, 'Failed while creating user'))
    .then(res => res.json())
    .then(res => httpHelper.checkOCSStatus(res, 'Failed while creating user'))
    .then(() => {
      if (displayName !== false && displayName !== null) {
        promiseList.push(
          new Promise((resolve, reject) => {
            const body = new URLSearchParams()
            body.append('key', 'display')
            body.append('value', displayName)
            const url = `cloud/users/${encodeURIComponent(userId)}`
            httpHelper.putOCS(url, 'admin', body).then(res => {
              if (res.status !== 200) {
                reject(new Error('Could not set display name of user'))
              }
              resolve(res)
            })
          })
        )
      }
    })
    .then(() => {
      if (email !== false && email !== null) {
        promiseList.push(
          new Promise((resolve, reject) => {
            const body = new URLSearchParams()
            body.append('key', 'email')
            body.append('value', email)
            const url = `cloud/users/${encodeURIComponent(userId)}`
            httpHelper.putOCS(url, 'admin', body).then(res => {
              if (res.status !== 200) {
                reject(new Error('Could not set email of user'))
              }
              resolve()
            })
          })
        )
      }
    })
    .then(() => {
      return Promise.all(promiseList)
    })
}

function deleteUser(userId) {
  userSettings.deleteUserFromCreatedUsersList(userId)
  const url = `cloud/users/${userId}`
  return httpHelper.deleteOCS(url)
}

function initUser(userId) {
  const url = `cloud/users/${userId}`
  return httpHelper.getOCS(url, userId)
}

/**
 *
 * @param {string} groupId
 * @returns {*|Promise}
 */
function createGroup(groupId) {
  const body = new URLSearchParams()
  body.append('groupid', groupId)
  userSettings.addGroupToCreatedGroupsList(groupId)
  const url = 'cloud/groups'
  return httpHelper.postOCS(url, 'admin', body)
}

/**
 *
 * @param {string} groupId
 * @returns {*|Promise}
 */
function deleteGroup(groupId) {
  userSettings.deleteGroupFromCreatedGroupsList(groupId)
  const url = `cloud/groups/${groupId}`
  return httpHelper.deleteOCS(url)
}

function addToGroup(userId, groupId) {
  const body = new URLSearchParams()
  body.append('groupid', groupId)
  const url = `cloud/users/${userId}/groups`
  return httpHelper.postOCS(url, 'admin', body)
}

Given('user {string} has been created with default attributes', async function(userId) {
  await deleteUser(userId)
  await createDefaultUser(userId)
  await initUser(userId)
})

Given('user {string} has been deleted', function(userId) {
  return deleteUser(userId)
})



Given('the quota of user {string} has been set to {string}', function(userId, quota) {
  const body = new URLSearchParams()
  body.append('key', 'quota')
  body.append('value', quota)
  const url = `cloud/users/${userId}`
  return httpHelper
    .putOCS(url, 'admin', body)
    .then(res => httpHelper.checkStatus(res, 'Could not set quota.'))
})

Given('these users have been created with default attributes but not initialized:', function(
  dataTable
) {
  return Promise.all(
    dataTable.rows().map(userId => {
      return deleteUser(userId.toString()).then(() => createDefaultUser(userId.toString()))
    })
  )
})

Given('these users have been created but not initialized:', function(dataTable) {
  codify.replaceInlineTable(dataTable)
  return Promise.all(
    dataTable.hashes().map(user => {
      const userId = user.username
      const password = user.password || userSettings.getPasswordForUser(userId)
      const displayName = user.displayname || false
      const email = user.email || false
      return deleteUser(userId).then(() => createUser(userId, password, displayName, email))
    })
  )
})

Given('these users have been created with default attributes:', function(dataTable) {
  return Promise.all(
    dataTable.rows().map(user => {
      const userId = user[0]
      return deleteUser(userId)
        .then(() => createDefaultUser(userId))
        .then(() => initUser(userId))
    })
  )
})

Given('group {string} has been created', function(groupId) {
  return deleteGroup(groupId.toString()).then(() => createGroup(groupId.toString()))
})

Given('these groups have been created:', function(dataTable) {
  return Promise.all(
    dataTable.rows().map(groupId => {
      return deleteGroup(groupId.toString()).then(() => createGroup(groupId.toString()))
    })
  )
})

Given('user {string} has been added to group {string}', function(userId, groupId) {
  return addToGroup(userId, groupId)
})

After(async function() {
  const createdUsers = Object.keys(userSettings.getCreatedUsers('LOCAL'))
  const createdRemoteUsers = Object.keys(userSettings.getCreatedUsers('REMOTE'))
  const createdGroups = userSettings.getCreatedGroups()

  await Promise.all([...createdUsers.map(deleteUser), ...createdGroups.map(deleteGroup)])
})

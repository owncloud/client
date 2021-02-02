const { BACKENDS } = require('./backendHelper')

const passwords = {
  admin: process.env.ADMIN_PASSWORD || 'admin',
  regular: process.env.REGULAR_USER_PASSWORD || '123456',
  alt1: process.env.ALT1_USER_PASSWORD || '1234',
  alt2: process.env.ALT2_USER_PASSWORD || 'AaBb2Cc3Dd4',
  alt3: process.env.ALT3_USER_PASSWORD || 'aVeryLongPassword42TheMeaningOfLife',
  alt4: process.env.ALT4_USER_PASSWORD || 'ThisIsThe4thAlternatePwd',
  alt11: process.env.ALT11_USER_PASSWORD || 'E-leven'
}

const adminUsername = process.env.ADMIN_USERNAME || 'admin'
module.exports = {
  passwords,
  // list of default users
  defaultUsers: {
    admin: {
      displayname: adminUsername,
      password: passwords.admin
    },
    regularuser: {
      displayname: 'Regular User',
      password: passwords.regular,
      email: 'regularuser@example.org'
    },
    user0: {
      displayname: 'Regular User',
      password: passwords.regular,
      email: 'user0@example.org'
    },
    user1: {
      displayname: 'User One',
      password: passwords.alt1,
      email: 'user1@example.org'
    },
    user2: {
      displayname: 'User Two',
      password: passwords.alt2,
      email: 'user2@example.org'
    },
    user3: {
      displayname: 'User Three',
      password: passwords.alt3,
      email: 'user3@example.org'
    },
    user4: {
      displayname: 'User Four',
      password: passwords.alt4,
      email: 'user4@example.org'
    },
    user11: {
      displayname: 'User Eleven',
      password: passwords.alt11,
      email: 'user11@example.org'
    },
    usergrp: {
      displayname: 'User Grp',
      password: passwords.regular,
      email: 'usergrp@example.org'
    },
    sharee1: {
      displayname: 'Sharee One',
      password: passwords.regular,
      email: 'sharee1@example.org'
    },
    // These users are available by default only in ocis backend when not using ldap
    Einstein: {
      displayname: 'Albert Einstein',
      password: 'relativity',
      email: 'einstein@example.org'
    },
    Richard: {
      displayname: 'Richard Feynman',
      password: 'superfluidity',
      email: 'richard@example.org'
    },
    Marie: {
      displayname: 'Marie Curie',
      password: 'radioactivity',
      email: 'marie@example.org'
    },
    Moss: {
      displayname: 'Maurice Moss',
      password: 'vista',
      email: 'moss@example.org'
    }
  },
  createdUsers: {},
  createdRemoteUsers: {},
  createdGroups: [],

  /**
   *
   * @param {string} userId
   * @param {string} password
   * @param {string} displayname
   * @param {string} email
   */
  addUserToCreatedUsersList: function(userId, password, displayname = null, email = null) {
      this.createdUsers[userId] = {
        password: password,
        displayname: displayname,
        email: email
    }
  },
  /**
   *
   * @param {string} userId
   */
  deleteUserFromCreatedUsersList: function(userId) {
    delete this.createdUsers[userId]
  },
  /**
   *
   * @param {string} groupId
   */
  addGroupToCreatedGroupsList: function(groupId) {
    this.createdGroups.push(groupId)
  },
  /**
   *
   * @param {string} userId
   */
  deleteGroupFromCreatedGroupsList: function(groupId) {
    this.createdGroups = this.createdGroups.filter(item => item !== groupId)
  },
  /**
   * gets the password of a previously created user
   * if the user was not created yet, gets the password from the default Users list
   * if the user is not in that list either, returns the userId as password
   *
   * @param {string} userId
   * @returns {string}
   */
  getPasswordForUser: function(userId) {
    if (userId in this.createdUsers) {
      return this.createdUsers[userId].password
    } else if (userId in this.defaultUsers) {
      return this.defaultUsers[userId].password
    } else {
      // user was not created yet and is not in the default users list, let the userId be the password
      return userId
    }
  },
  /**
   * gets the display name of a previously created user
   * if the user was not created yet, gets the display name from the default Users list
   * if the user is not in that list either, returns the userId as display name
   *
   * @param {string} userId
   * @returns {string}
   */
  getDisplayNameForUser: function(userId) {
    let user = {}
    if (userId in this.createdUsers) {
      user = this.createdUsers[userId]
    } else if (userId in this.defaultUsers) {
      user = this.defaultUsers[userId]
    } else {
      return userId
    }
    if ('displayname' in user && user.displayname !== null) {
      return user.displayname
    } else {
      return userId
    }
  },

  /**
   * gets the username of a previously created user from displayName
   *
   * @param {string} displayName
   */
  getUsernameFromDisplayname: function(displayName) {
    for (const userid in this.createdUsers) {
      if (this.createdUsers[userid].displayname === displayName) {
        return userid
      }
    }
  },
  /**
   * gets the display name of the specified user from the default users list
   * returns null if the user is not in that list
   *
   * @param {string} userId
   * @returns {null|string}
   */
  getDisplayNameOfDefaultUser: function(userId) {
    if (userId in this.defaultUsers) {
      return this.defaultUsers[userId].displayname
    } else {
      return null
    }
  },
  /**
   * gets the email address of a previously created user
   * if the user was not created yet, gets the email address from the default Users list
   * if the user is not in that list either, returns null
   *
   * @param {string} userId
   * @returns {null|string}
   */
  getEmailAddressForUser: function(userId) {
    let user = {}
    if (userId in this.createdUsers) {
      user = this.createdUsers[userId]
    } else if (userId in this.defaultUsers) {
      user = this.defaultUsers[userId]
    } else {
      return `${userId}@example.org`
    }
    if ('email' in user && user.email !== null) {
      return user.email
    } else {
      return `${userId}@example.org`
    }
  },
  /**
   * gets the email address of the specified user from the default users list
   * returns null if the user is not in that list
   *
   * @param {string} userId
   * @returns {null|string}
   */
  getEmailAddressOfDefaultUser: function(userId) {
    if (userId in this.defaultUsers) {
      return this.defaultUsers[userId].email
    } else {
      return null
    }
  },
  /**
   *
   * @returns {module.exports.createdUsers|{}}
   */
  getCreatedUsers: function(server = BACKENDS.local) {
    switch (server) {
      case BACKENDS.local:
        return this.createdUsers
      case BACKENDS.remote:
        return this.createdRemoteUsers
      default:
        throw new Error('Invalid value for server. want = "REMOTE"/"LOCAL", got = ' + server)
    }
  },
  /**
   *
   * @returns {Array}
   */
  getCreatedGroups: function() {
    return this.createdGroups
  },

  resetCreatedUsers: function() {
    this.createdUsers = {}
  },

  resetCreatedGroups: function() {
    this.createdGroups = []
  }
}

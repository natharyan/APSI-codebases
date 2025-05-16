import mongoose from 'mongoose'

export const user = new mongoose.Schema(
  {
    username: {
      type: String,
      required: [true, 'Please provide a unique Username'],
      unique: true,
    },
    password: {
      type: String,
      required: [true, 'Please provide a password'],
      unique: false,
    },
    email: {
      type: String,
      required: [true, 'Please provide a unique email'],
      unique: true,
    },
    firstName: {
      type: String,
      default: null,
    },
    lastName: {
      type: String,
      default: null,
    },
    mobile: {
      type: Number,
      default: null,
    },
    address: {
      type: String,
      default: null,
    },
    profile: {
      type: String,
      default: null,
    },
    role: {
      type: String,
      enum: ['admin', 'user'],
      default: 'user',
    },
    verifiedUser: {
      type: Boolean,
      default: false,
    },
    verificationToken: {
      type: String || null,
      default: null,
    },
    verifiedDate: {
      type: Date,
      default: null,
    },
    tokenExpirationDate: {
      type: Date,
      default: null,
    },
  },
  {
    collection: 'usercollection',
    timestamps: true,
  }
)

const UserSchema = mongoose.model('user', user)

export { UserSchema }

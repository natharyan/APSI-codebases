import { Schema, Types, model } from 'mongoose'

const TokenSchema = new Schema(
  {
    refreshToken: { type: String, required: true },
    // userAgent: { type: String, required: true },
    isValid: { type: Boolean, default: true },
    user: {
      type: Types.ObjectId,
      ref: 'User',
      required: true,
    },
  },
  {
    collection: 'tokencollection',
    timestamps: true,
  }
)

export default model('Token', TokenSchema)

import jwt from 'jsonwebtoken'

const JWT_SECRET: string = process.env.JWT_SECRET || ''

const createJWT = ({ payload }: any) => {
  const token = jwt.sign(payload, JWT_SECRET)
  return token
}

const isTokenValid = (token: any) => jwt.verify(token, JWT_SECRET)

const attachCookiesToResponse = ({ res, user, refreshToken }: any) => {
  const accessTokenJWT = createJWT({ payload: { user } })
  const refreshTokenJWT = createJWT({ payload: { user, refreshToken } })

  const oneDay = 1000 * 60 * 60 * 24
  const longerExp = 1000 * 60 * 60 * 24 * 30

  res.cookie('accessToken', accessTokenJWT, {
    httpOnly: true,
    secure: process.env.NODE_ENV === 'production',
    signed: true,
    expires: new Date(Date.now() + oneDay),
  })

  res.cookie('refreshToken', refreshTokenJWT, {
    httpOnly: true,
    secure: process.env.NODE_ENV === 'production',
    signed: true,
    expires: new Date(Date.now() + longerExp),
  })
}
// const attachSingleCookieToResponse = ({ res, user }) => {
//   const token = createJWT({ payload: user })

//   const oneDay = 1000 * 60 * 60 * 24

//   res.cookie('token', token, {
//     httpOnly: true,
//     expires: new Date(Date.now() + oneDay),
//     secure: process.env.NODE_ENV === 'production',
//     signed: true,
//   })
// }

export {
  createJWT,
  isTokenValid,
  attachCookiesToResponse,
}

